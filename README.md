# web server themis 2.0 version

## description
    themis is a web server implemented with libevent with c++, featuring a fully asynchronized web service routine with builtin sql support. 

## modules
there are severial modules constituting the server

- network module 
    
    the network module bind with libevent to handle the underlying IO logic. Reactor is the IO implementation.

- sql module

    provide the user with sql driver and connection pool. User must manually initialize driver to take effect, otherwise the query will not succeed.

- event module

    features a promise api similar to the ES6 promise api. All operation including the controller http response will and should act asynchronizely.

- protocol module

    this module work with reactor to handle the data input, and support protocol user extension.