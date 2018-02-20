#ifndef TIMER_QUERIES_H
#define TIMER_QUERIES_H

// Timing code adapted from http://www.lighthouse3d.com/tutorials/opengl-timer-query/
class TimerQueries {
public:
	enum QueryType { VOXELIZE_TIME, SHADOWMAP_TIME, RADIANCE_TIME, RENDER_TIME, QUERY_COUNT };

	TimerQueries() {
		glGenQueries(QUERY_COUNT, queryID[queryBackBuffer]);
		glGenQueries(QUERY_COUNT, queryID[queryFrontBuffer]);

		// Put something in front buffer to avoid warnings when accessing on first frame
		for (int i = 0; i < QUERY_COUNT; i++) {
			glBeginQuery(GL_TIME_ELAPSED, queryID[queryFrontBuffer][i]);
			glEndQuery(GL_TIME_ELAPSED);
		}
	}

	~TimerQueries() {
		glDeleteQueries(QUERY_COUNT, queryID[queryBackBuffer]);
		glDeleteQueries(QUERY_COUNT, queryID[queryFrontBuffer]);
	}

	TimerQueries(const TimerQueries &other) = delete;
	TimerQueries &operator=(const TimerQueries &other) = delete;
	TimerQueries(TimerQueries &&other) = delete;
	TimerQueries &operator=(TimerQueries &&other) = delete;

	void beginQuery(QueryType type) { glBeginQuery(GL_TIME_ELAPSED, queryID[queryBackBuffer][type]); }

	void endQuery() { glEndQuery(GL_TIME_ELAPSED); }

	void getQueriesAndSwap() {
		for (int i = 0; i < QUERY_COUNT; i++) {
			glGetQueryObjectui64v(queryID[queryFrontBuffer][i], GL_QUERY_RESULT, &time[i]);
		}
		swapBuffers();
	}

	// Get the time in milliseconds
	double getTime(QueryType type) {
		return time[type] / 1.0e6;
	}

	void swapBuffers() {
		if (queryBackBuffer) {
			queryBackBuffer = 0;
			queryFrontBuffer = 1;
		}
		else {
			queryBackBuffer = 1;
			queryFrontBuffer = 0;
		}
	}

private:
	static const unsigned QUERY_BUFFERS = 2;
	unsigned queryID[QUERY_BUFFERS][QUERY_COUNT];
	unsigned queryBackBuffer = 0, queryFrontBuffer = 1;
	GLuint64 time[QUERY_COUNT];
};

#endif