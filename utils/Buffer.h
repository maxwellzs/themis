#ifndef Buffer_h
#define Buffer_h

#include <event2/event.h>
#include <cstddef>
#include <list>
#include <vector>
#include <cstdint>
#include <string>

namespace themis {

    class BufferWriter;
    class BufferReader;

    /**
     * @brief a buffer holds a connection buffer
     *
     */
    class Buffer {
        friend BufferWriter;
        friend BufferReader;

    private:
        std::list<std::vector<uint8_t>> chunks;

        using ChunkIterator = std::list<std::vector<uint8_t>>::iterator;
        using ChunkIteratorPtr = ChunkIterator*;

        const size_t SIZE_PER_CHUNK;
        size_t writeIndex = 0; // the write index in the last chunk, always pointing to a valid chunk
        size_t readIndex = 0;  // the read index in the first chunk
        /**
         * @brief allocate a new chunk and put to the back of the list
         *
         */
        void allocateChunk();

    public:
        /**
         * @brief Construct a new Buffer, each chunk with default size 1024
         *
         * @param chunkSize the size of chunk
         */
        Buffer(size_t chunkSize = 1024);
        /**
         * @brief Move construct a buffer with an existing one, the existing one must not be used anymore
         * 
         * @param b original buffer
         */
        Buffer(Buffer&& b) : chunks(std::move(b.chunks)), 
        readIndex(b.readIndex), writeIndex(b.writeIndex), SIZE_PER_CHUNK(b.SIZE_PER_CHUNK) {}

        bool empty() {
            return readIndex == writeIndex && chunks.size() == 1;
        }

        /**
         * @brief delete all chunks
         * 
         */
        void clear() {
            chunks.clear();
        }
    };

    /**
     * @brief a BufferWriter decorate the Buffer object and write bytes into it
     *
     */
    class BufferWriter {
    private:
        Buffer &buffer;

    public:
        /**
         * @brief Construct a new Buffer Writer object decorating the given buffer
         *
         * @param b buffer to decorate
         */
        BufferWriter(Buffer &b);

        /**
         * @brief consume as much bytes from the socket as possible, if error occurred,
         * an exception will be casted
         *
         * @param socket socket
         */
        void receiveFrom(evutil_socket_t socket);

        /**
         * @brief write a string to base buffer
         * 
         * @param s 
         */
        void write(const std::string& s);

        /**
         * @brief attemp to put @param count number of byte to the buffer
         * 
         * @param src source pointer
         * @param count how many bytes
         */
        void write(const void* src, size_t count);
    };

    /**
     * @brief a BufferReader decorate a Buffer object and consume data from it
     *
     */
    class BufferReader {
    private:
        Buffer &buffer;

        size_t originalReadIndex;
        Buffer::ChunkIterator current; // the chunk being currently consumed
    public:
        /**
         * @brief Destroy the Buffer Reader means the buffer chunk consumed will be remove 
         * from the list of chunk
         * 
         */
        ~BufferReader();
        /**
         * @brief Construct a new Buffer Reader object decorating the given buffer
         * 
         * @param b buffer to decorate
         */
        BufferReader(Buffer& b);

        /**
         * @brief send out as much bytes as possible from this buffer
         * 
         * @param socket target socket
         * @return true if should send again
         */
        bool sendTo(evutil_socket_t socket);

        /**
         * @brief get a line from the buffer
         * 
         */
        void getline(std::string& out);
        
        /**
         * @brief try to extract count number of bytes to out buffer
         * note that the out buffer must have enough space to hold up to 
         * count byte otherwise there will be undefined behaviour
         * 
         * @param out 
         * @param count 
         * @return number bytes acquired
         */
        size_t getBytes(std::vector<uint8_t>& out, size_t count);
        /**
         * @brief attempt to put @param count amount of bytes to the dest location
         * and return the size actually read
         * 
         * @param dest 
         * @param count 
         * @return size_t 
         */
        size_t getBytes(void* dest, size_t count);

        /**
         * @brief reset current chunk and read position
         * 
         */
        void revert();
        /**
         * @brief clear the chunks that has been consumed
         * 
         */
        void finialize();
    };

} // namespace themis

#endif
