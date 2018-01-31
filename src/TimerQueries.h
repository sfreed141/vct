#ifndef TIMER_QUERIES_H
#define TIMER_QUERIES_H

// Timing code adapted from http://www.lighthouse3d.com/tutorials/opengl-timer-query/
class TimerQueries {
public:
	enum QueryType { VOXELIZE_TIME, RENDER_TIME, QUERY_COUNT };

	TimerQueries() {
		glGenQueries(QUERY_COUNT, queryID[queryBackBuffer]);
		glGenQueries(QUERY_COUNT, queryID[queryFrontBuffer]);

		// Put something in front buffer to avoid warnings when accessing on first frame
		glBeginQuery(GL_TIME_ELAPSED, queryID[queryFrontBuffer][VOXELIZE_TIME]);
		glEndQuery(GL_TIME_ELAPSED);
		glBeginQuery(GL_TIME_ELAPSED, queryID[queryFrontBuffer][RENDER_TIME]);
		glEndQuery(GL_TIME_ELAPSED);
	}

	~TimerQueries() {
		glDeleteQueries(QUERY_COUNT, queryID[queryBackBuffer]);
		glDeleteQueries(QUERY_COUNT, queryID[queryFrontBuffer]);
	}

	void beginQuery(QueryType type) { glBeginQuery(GL_TIME_ELAPSED, queryID[queryBackBuffer][type]); }

	void endQuery() { glEndQuery(GL_TIME_ELAPSED); }

	void getQueriesAndSwap() {
		glGetQueryObjectui64v(queryID[queryFrontBuffer][VOXELIZE_TIME], GL_QUERY_RESULT, &time[VOXELIZE_TIME]);
		glGetQueryObjectui64v(queryID[queryFrontBuffer][RENDER_TIME], GL_QUERY_RESULT, &time[RENDER_TIME]);
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