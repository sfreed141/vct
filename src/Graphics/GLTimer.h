#ifndef GLTIMER_H
#define GLTIMER_H

#include "opengl.h"

class GLTimer {
public:
    GLTimer() {
        glGenQueries(2, id);

        // Put something in buffers to avoid warnings when accessing the first time.
        start();
        stop();
    }

    ~GLTimer() {
        glDeleteQueries(2, id);
    }

    GLTimer(const GLTimer &other) = delete;
    GLTimer &operator=(const GLTimer &other) = delete;
    GLTimer(GLTimer &&other) = delete;
    GLTimer &operator=(GLTimer &&other) = delete;

    void start() {
        glQueryCounter(id[startId], GL_TIMESTAMP);
    }

    void stop() {
        glQueryCounter(id[stopId], GL_TIMESTAMP);
    }

    void getQueryResult() {
        glGetQueryObjectui64v(id[startId], GL_QUERY_RESULT, &startTime);
        glGetQueryObjectui64v(id[stopId], GL_QUERY_RESULT, &stopTime);
    }

    // Returns query time (in nanoseconds)
    double getTime() const {
        return stopTime - startTime;
    }

private:
    const unsigned char startId = 0, stopId = 1;
    GLuint id[2];
    GLuint64 startTime, stopTime;
};

class GLBufferedTimer {
public:
    GLBufferedTimer() = default;
    ~GLBufferedTimer() = default;

    GLBufferedTimer(const GLBufferedTimer &other) = delete;
    GLBufferedTimer &operator=(const GLBufferedTimer &other) = delete;
    GLBufferedTimer(GLBufferedTimer &&other) = delete;
    GLBufferedTimer &operator=(GLBufferedTimer &&other) = delete;

    void start() {
        timers[backBuffer].start();
    }

    void stop() {
        timers[backBuffer].stop();
    }

    void getQueryResult() {
        timers[frontBuffer].getQueryResult();
        time = timers[frontBuffer].getTime();
        swapBuffers();
    }

    // Returns query time (in nanoseconds)
    double getTime() const {
        return time;
    }

private:
    unsigned char backBuffer = 0, frontBuffer = 1;
    GLTimer timers[2];
    GLuint64 time;

    void swapBuffers() {
        frontBuffer = backBuffer;
        backBuffer = 1 - backBuffer;
    }
};

#endif
