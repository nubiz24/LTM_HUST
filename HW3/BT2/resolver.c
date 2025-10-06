#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>

static int is_ipv4_like_string(const char *s) {
	if (s == NULL || *s == '\0') return 0;
	for (const char *p = s; *p; ++p) {
		if (!isdigit((unsigned char)*p) && *p != '.') return 0;
	}
	return 1;
}

static void print_official_and_alias_ips_from_hostent(const struct hostent *he) {
	if (!he || !he->h_addr_list || !he->h_addr_list[0]) return;
	struct in_addr addr;
	memcpy(&addr, he->h_addr_list[0], sizeof(addr));
	printf("Official IP: %s\n", inet_ntoa(addr));

	printf("Alias IP:\n");
	for (int i = 1; he->h_addr_list[i] != NULL; ++i) {
		memcpy(&addr, he->h_addr_list[i], sizeof(addr));
		printf("%s\n", inet_ntoa(addr));
	}
}

static void reverse_lookup_ipv4(const char *ip_str) {
	struct in_addr addr4;
	if (inet_pton(AF_INET, ip_str, &addr4) != 1) {
		if (is_ipv4_like_string(ip_str)) {
			printf("Invalid address\n");
		} else {
			printf("Not found information\n");
		}
		return;
	}

	struct hostent *he = gethostbyaddr((const void *)&addr4, sizeof(addr4), AF_INET);
	if (!he) {
		printf("Not found information\n");
		return;
	}

	printf("Official name: %s\n", he->h_name);
	printf("Alias name:\n");
	if (he->h_aliases) {
		for (char **alias = he->h_aliases; *alias != NULL; ++alias) {
			printf("%s\n", *alias);
		}
	}
}

static void forward_lookup_hostname(const char *hostname) {
	struct hostent *he = gethostbyname(hostname);
	if (!he || he->h_addrtype != AF_INET) {
		printf("Not found information\n");
		return;
	}
	print_official_and_alias_ips_from_hostent(he);
}

static void process_parameter(const char *param) {
	if (!param || *param == '\0') return;

	struct in_addr dummy4;
	if (inet_pton(AF_INET, param, &dummy4) == 1) {
		reverse_lookup_ipv4(param);
		return;
	}

	if (is_ipv4_like_string(param)) {
		printf("Invalid address\n");
		return;
	}

	forward_lookup_hostname(param);
}

int main(int argc, char *argv[]) {
	if (argc == 2) {
		process_parameter(argv[1]);
		return 0;
	}

	char buffer[1024];
	while (1) {
		if (!fgets(buffer, sizeof(buffer), stdin)) {
			break;
		}
		size_t len = strlen(buffer);
		while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
			buffer[--len] = '\0';
		}
		char *start = buffer;
		while (*start && isspace((unsigned char)*start)) start++;
		while (len > 0 && isspace((unsigned char)buffer[len - 1])) {
			buffer[--len] = '\0';
		}
		if (*start == '\0') {
			break;
		}
		process_parameter(start);
	}
	return 0;
}
