#include <stddef.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>


#define MAX_TEXT 300000000
#define MAX_PAT  4096

static char text[MAX_TEXT];
static char pat[MAX_PAT];



typedef struct{
	const char* text;
	size_t len_text;
	const char* pat;
	size_t len_pat;

	size_t start;
	size_t end;

	size_t count;
} ThreadArgs;


size_t count_matches(const char* text, size_t len_text, const char* pat, size_t len_pat, size_t start, size_t end) {
	size_t c = 0;

	if (len_pat == 0 || len_pat > len_text)
		return 0;


	size_t n_pos = len_text - len_pat + 1;
	if (end > n_pos) end = n_pos;

	for(size_t i = start; i < end; i++){
		int flag = 1;
		for(size_t j = 0; j < len_pat; j++) {
			if (text[i + j] != pat[j]) {
				flag = 0;
				break;
			}
		}
		if (flag == 1) c += 1;
	}
	return c;
}

void* thread_search(void* _args){
	ThreadArgs* args = (ThreadArgs*) _args;
	args->count = count_matches(args->text, args->len_text, args->pat, args->len_pat, args->start, args->end);

	return NULL;
}


ssize_t read_all(int fd, char* buffer, size_t max) {
	size_t len = 0;
	while (len < max) {
		ssize_t r = read(fd, buffer + len, max - len);
		if (r == 0) break;
		else if (r == -1) return -1;
		len += (size_t)r;
	}

	if (len == max) return -1;
	buffer[len] = '\0';
	return len;
}


int parse_threads(const char* s, int* res) {
	if (s == NULL || res == NULL || (s[0] == '\0')) {
		write(2, "incorrect argument (threads)\n", 29);
		return -1;
	}

	int v = 0;
	for (int i = 0; s[i] != '\0'; i++) {
		char c = s[i];

		if (!(c >= '0' && c <= '9' )) {
			write(2, "nan\n", 4);
			return -1;
		}

		int digit = c - '0';
		if (v > (1024 - digit) / 10) {
			write(2, "overflow\n", 9);
			return -1;
		}

		v = 10 * v + digit;
	}

	if (v == 0) {
		write(2, "number of threads should be > 0\n", 32);
		return -1;
	}

	*res = v;
	return 0;
}


long long now_ns(void) {
	struct timespec ts;
	int result = clock_gettime(CLOCK_MONOTONIC, &ts);
	if (result != 0) return -1;

	return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

void put_num(unsigned long long v) {
	char b[20]; int i = 20;

	if (v == 0)  b[--i] = '0';

	while (v > 0) {
		b[--i] = '0' + v % 10;
        v /= 10;
	}
	write(1, b + i, 20 - i);
}


void put_ms(unsigned long long ns) {
	put_num(ns / 1000000);
	write(1, ".", 1);

	unsigned long long frac = (ns / 1000) % 1000;
	char frac_str[3] = {
		'0' + frac / 100,
		'0' + (frac / 10) % 10,
		'0' + frac % 10
	};

	write(1, frac_str, 3);
}


int main(int argc, char* argv[]) {
	if (argc != 4) {
		return 1;
	}

	int num_threads;
	int parsing_num = parse_threads(argv[3], &num_threads);
	if (parsing_num == -1) return 1;


	int text_fd = open(argv[1], O_RDONLY);
	if (text_fd == -1)
		return 1;

	int pat_fd = open(argv[2], O_RDONLY);
	if (pat_fd == -1) {
		close(text_fd);
		return 1;
	}

	ssize_t len_text = read_all(text_fd, text, MAX_TEXT);
	ssize_t len_pat  = read_all(pat_fd,  pat,  MAX_PAT);
	if (len_text < 0 || len_pat < 0) {
		close(text_fd);
		close(pat_fd);
		return 1;

	}

	close(text_fd);
	close(pat_fd);


	if (len_pat > 0 && pat[len_pat - 1] == '\n') len_pat--;

	size_t count = 0;
	size_t n_pos = 0;

	if (len_pat > 0 && (size_t)len_pat <= (size_t)len_text)
		n_pos = (size_t)len_text - (size_t)len_pat + 1;


	long long t0 = now_ns();
	count = count_matches(text, (size_t)len_text, pat, (size_t)len_pat, 0, n_pos);
	long long t1 = now_ns();

	if (t0 < 0 || t1 < 0) {
		write(2, "clock error\n", 12);
		return 1;
	}

	unsigned long long dt = (unsigned long long)(t1 - t0);


	write(1, "sequential: count=", 18);
	put_num(count);
	write(1, " time_ms=", 9);
	put_ms(dt);
	write(1, "\n", 1);


	pthread_t threads[1024];
	ThreadArgs args[1024];

	size_t chunk = n_pos/num_threads;
	size_t rem = n_pos % num_threads;
	size_t next = 0;
	size_t created = 0;
	int create_failed = 0;

	long long tp0 = now_ns();

	for (size_t i = 0; i < (size_t)num_threads; i++) {
		size_t block = chunk + ((i < rem) ? 1 : 0);

		args[i].text = text;
		args[i].len_text = (size_t)len_text;
		args[i].pat = pat;
		args[i].len_pat = (size_t)len_pat;

		args[i].start = next;
		args[i].end = next + block;

		args[i].count = 0;

		next = args[i].end;

		int result = pthread_create(&threads[i], NULL, thread_search, &args[i]);

		if (result != 0) {
			create_failed = 1;
			break;
		}
		created++;
	}



	int join_failed = 0;

	for (size_t i = 0; i < created; i++) {
		int result = pthread_join(threads[i], NULL);
		if (result != 0) {
			join_failed = 1;
		}
	}

	if (create_failed || join_failed) {
		write(2, "thread error\n", 13);
		return 1;
	}
	size_t par_count = 0;

	for (size_t i = 0; i < created; i++) {
		par_count += args[i].count;
	}

	long long tp1 = now_ns();

	if (tp0 < 0 || tp1 < 0) {
		write(2, "clock error\n", 12);
		return 1;
	}

	unsigned long long par_dt = (unsigned long long)(tp1 - tp0);
	write(1, "parallel: threads=", 18);
	put_num((unsigned long long)num_threads);
	write(1, " count=", 7);
	put_num(par_count);
	write(1, " time_ms=", 9);
	put_ms(par_dt);
	if (par_count == count) write(1, "\n", 1);
	else { write(1, "mismatch\n", 10); return 1; }

	return 0;
}