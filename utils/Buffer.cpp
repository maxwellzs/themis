#include "Buffer.h"
#include <unistd.h>
#include <cstring>

void themis::Buffer::allocateChunk() {
    chunks.emplace_back(std::vector<uint8_t>(SIZE_PER_CHUNK));
}

themis::Buffer::Buffer(size_t chunkSize) :
    SIZE_PER_CHUNK(chunkSize) {
    allocateChunk();
}

themis::BufferWriter::BufferWriter(Buffer &b) 
: buffer(b) {

}

void themis::BufferWriter::receiveFrom(evutil_socket_t socket) {
    for(;;) {
        auto& lastChunk = buffer.chunks.back();
        size_t capacity = lastChunk.size() - buffer.writeIndex;
        ssize_t recvCount = read(socket, &lastChunk.data()[buffer.writeIndex], capacity);
        
        // peer disconnected
        if(recvCount == 0) throw std::exception();

        // error, may throw more details later
        if(recvCount == -1) {
            // no more data available
            if(errno == EAGAIN) break;
            throw std::exception();
        }

        buffer.writeIndex += recvCount;
        if(buffer.writeIndex != buffer.SIZE_PER_CHUNK) {
            // not a full chunk, means there are no more data temporarily
            break;
        }
        // otherwise a new chunk is allocated
        buffer.allocateChunk();
        buffer.writeIndex = 0;
    }
}

void themis::BufferWriter::write(const std::string &s) {
    write(s.data(), s.length());
}

void themis::BufferWriter::write(const void *src, size_t count) {
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(src);
    for(size_t copied = 0; copied < count; ) {
        auto& lastChunk = buffer.chunks.back();
        size_t capacity = lastChunk.size() - buffer.writeIndex;
        size_t remain = count - copied;
        size_t copySize = capacity > remain ? remain : capacity;
        
        std::memcpy(lastChunk.data() + buffer.writeIndex, ptr + copied, copySize);
        copied += copySize;
        buffer.writeIndex += copySize;
        
        if(buffer.writeIndex == buffer.SIZE_PER_CHUNK) {
            buffer.allocateChunk();
            buffer.writeIndex = 0;
        }
    }
}

themis::BufferReader::~BufferReader() {
    finialize();
}

themis::BufferReader::BufferReader(Buffer &b) :
    buffer(b), current(b.chunks.begin()), originalReadIndex(b.readIndex) {
}

bool themis::BufferReader::sendTo(evutil_socket_t socket) {
    for(;;) {

        if(buffer.readIndex == buffer.SIZE_PER_CHUNK) {
            ++current;
            buffer.readIndex = 0;
        }

        // there are no more data in this buffer
        if(current == buffer.chunks.end() ||
        (current == --buffer.chunks.end() && buffer.readIndex == buffer.writeIndex)) return false;

        size_t sendSize;
        if(current == --buffer.chunks.end()) {
            // last chunk
            sendSize = buffer.writeIndex - buffer.readIndex;
        } else {
            sendSize = buffer.SIZE_PER_CHUNK - buffer.readIndex;
        }
        ssize_t result = write(socket, (*current).data() + buffer.readIndex, sendSize);
        if(result != sendSize) {
            if(result == -1 && errno != EAGAIN) throw std::exception();

            // send again later
            buffer.readIndex += sendSize;
            return true;
        }
       
        buffer.readIndex += result;
    }
    return false;
}

void themis::BufferReader::getline(std::string &out) {
    // peek for LF
    size_t lineSize = 0;
    Buffer::ChunkIterator beginChunk = current;
    size_t beginIndex = buffer.readIndex;
    for(;; ++buffer.readIndex, ++lineSize) {
        // move to next chunk
        if(buffer.readIndex == buffer.SIZE_PER_CHUNK) {
            ++current;
            buffer.readIndex = 0;
        }
        // buffer ran out
        if(current == buffer.chunks.end() ||
        (current == --buffer.chunks.end() && buffer.readIndex == buffer.writeIndex)) throw std::exception();
        

        if((*current).at(buffer.readIndex) == '\n') {
            // found LF
            break;
        }
    }
    out.clear();
    if(lineSize == 0) {
        ++buffer.readIndex;
        return;
    }
    size_t copySize = 0;
    out.resize(lineSize);
    Buffer::ChunkIterator it = beginChunk;
    while(copySize < lineSize) {
        size_t s;
        if(it == beginChunk) {
            // first chunk
            if(it != current) {
                s = buffer.SIZE_PER_CHUNK - beginIndex;
            } else {
                // also last chunk
                s = buffer.readIndex - beginIndex;
            }
            std::memcpy(out.data() + copySize, (*it).data() + beginIndex, s);
        } else if (it == current) {
            // last chunk
            s = buffer.readIndex;
            std::memcpy(out.data() + copySize, (*it).data(), s);
        } else {
            // middle chunk
            s = buffer.SIZE_PER_CHUNK;
            std::memcpy(out.data() + copySize, (*it).data(), s);
        }
        // copy next chunk
        copySize += s;
        ++it;
    }
    // remove CR if present
    if(out.back() == '\r') out.pop_back();
    // advance readIndex once more
    ++buffer.readIndex;
}

size_t themis::BufferReader::getBytes(std::vector<uint8_t> &out, size_t count) {
    return getBytes(out.data(), count);
}

size_t themis::BufferReader::getBytes(void *dest, size_t count) {
    uint8_t* ptr = reinterpret_cast<uint8_t *>(dest);
    size_t acquired = 0;
    while(acquired < count) {

        if(buffer.readIndex == buffer.SIZE_PER_CHUNK) {
            ++current;
            buffer.readIndex = 0;
        }

        if(current == buffer.chunks.end() ||
        (current == --buffer.chunks.end() && buffer.readIndex == buffer.writeIndex)) return acquired;

        size_t await = count - acquired;
        size_t s;
        if(current == --buffer.chunks.end()) {
            // last chunk
            s = buffer.writeIndex - buffer.readIndex > await ? await : (buffer.writeIndex - buffer.readIndex);
        } else {
            // middle chunks or first chunk
            s = buffer.SIZE_PER_CHUNK - buffer.readIndex > await ? await : (buffer.SIZE_PER_CHUNK - buffer.readIndex);
        }
        std::memcpy(ptr + acquired, (*current).data() + buffer.readIndex, s);

        buffer.readIndex += s;
        acquired += s;
    }
    return acquired;
}

void themis::BufferReader::revert() {
    current = buffer.chunks.begin();
    buffer.readIndex = originalReadIndex;
}

void themis::BufferReader::finialize() {
    // otherwise 
    if(current == buffer.chunks.begin()) return;
    buffer.chunks.erase(buffer.chunks.begin(), current);
    originalReadIndex = buffer.readIndex;
}
