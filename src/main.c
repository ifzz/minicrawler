#define _GNU_SOURCE // memmem(.), strchrnul needs this :-(
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "h/minicrawler.h"
#include "h/proto.h"
#include "h/version.h"

struct ssettings settings;

/** nacte url z prikazove radky do struktur
 */
struct surl *initurls(int argc, char *argv[])
{
	struct surl *url, *curl, *purl;
	char *post = NULL, *p, *q;
	struct cookie cookies[COOKIESTORAGESIZE];
	int ccnt = 0, i = 0;

	url = (struct surl *)malloc(sizeof(struct surl));
	memset(url, 0, sizeof(struct surl));
	curl = url;

	for (int t = 1; t < argc; ++t) {

		// options
		if(!strcmp(argv[t], "-d")) {settings.debug=1;continue;}
		if(!strcmp(argv[t], "-S")) {settings.non_ssl=1;continue;}
		if(!strcmp(argv[t], "-h")) {settings.writehead=1;continue;}
		if(!strcmp(argv[t], "-i")) {settings.impatient=1;continue;}
		if(!strcmp(argv[t], "-p")) {settings.partial=1;continue;}
		if(!strcmp(argv[t], "-c")) {settings.convert=settings.convert_to_utf=1;continue;}
		if(!strcmp(argv[t], "-8")) {settings.convert_to_utf=1;continue;}
		if(!strcmp(argv[t], "-g")) {settings.gzip=1;continue;}
		if(!strncmp(argv[t], "-t", 2)) {settings.timeout=atoi(argv[t]+2);continue;}
		if(!strncmp(argv[t], "-D", 2)) {settings.delay=atoi(argv[t]+2);debugf("Delay time: %d\n",settings.delay);continue;}
		if(!strncmp(argv[t], "-w", 2)) {safe_cpy(settings.customheader, argv[t+1], I_SIZEOF(settings.customheader));t++;debugf("Custom header for all: %s\n",settings.customheader);continue;}
		if(!strncmp(argv[t], "-A", 2)) {str_replace((char *)&settings.customagent, argv[t+1], "%version%", VERSION); t++; debugf("Custom agent: %s\n",settings.customagent); continue;}
		if(!strncmp(argv[t], "-b", 2)) {
			p = argv[t+1];
			while (p[0] != '\0' && ccnt < COOKIESTORAGESIZE) {
				q = strchrnul(p, '\n');
				cookies[ccnt].name = malloc(q-p);
				cookies[ccnt].value = malloc(q-p);
				cookies[ccnt].domain = malloc(q-p);
				cookies[ccnt].path = malloc(q-p);
				sscanf(p, "%s\t%d\t%s\t%d\t%d\t%s\t%s", cookies[ccnt].domain, &cookies[ccnt].host_only, cookies[ccnt].path, &cookies[ccnt].secure, &cookies[ccnt].expire, cookies[ccnt].name, cookies[ccnt].value);
				p = (q[0] == '\n') ? q + 1 : q;
				ccnt++;
			}
			t++;
			continue;
		}
		if(!strcmp(argv[t], "-6")) {settings.ipv6=1;continue;}

		// urloptions
		if(!strcmp(argv[t], "-P")) {
			post = malloc(strlen(argv[t+1]) + 1);
			memcpy(post, argv[t+1], strlen(argv[t+1]) + 1);
			t++;
			debugf("[%d] POST: %s\n",i,post);
			continue;
		}
		if(!strncmp(argv[t], "-C", 2)) {safe_cpy(curl->customparam, argv[t+1], I_SIZEOF(curl->customparam));t++;debugf("[%d] Custom param: %s\n",i,curl->customparam);continue;}
		if(!strcmp(argv[t], "-X")) {safe_cpy(curl->method, argv[t+1], I_SIZEOF(curl->method)); t++; continue;}

		init_url(curl, argv[t], i++, post, cookies, ccnt);
		post = NULL;
		purl = curl;
		curl = (struct surl *)malloc(sizeof(struct surl));
		purl->next = curl;
	}

	free(curl);
	purl->next = NULL;

	for (int t = 0; t < ccnt; t++) {
		free(cookies[t].name);
		free(cookies[t].value);
		free(cookies[t].domain);
		free(cookies[t].path);
	}

	return url;
}

/** zpracuje signál (vypíše ho a ukončí program s -1)
 */
void sighandler(int signum)
{
	debugf("Caught signal %d\n",signum);
	exit(-1);
}


/** vypise napovedu
 */
void printusage()
{
	printf("\nminicrawler, version %s\n\nUsage:   minicrawler [options] [urloptions] url [[url2options] url2]...\n\n"
	         "Where\n"
	         "   options:\n"
	         "         -d         enable debug messages (to stderr)\n"
	         "         -tSECONDS  set timeout (default is 5 seconds)\n"
	         "         -h         enable output of HTTP headers\n"
	         "         -i         enable impatient mode (minicrawler exits few seconds earlier if it doesn't make enough progress)\n"
	         "         -p         output also URLs that timed out and a reason of it\n"
	         "         -A STRING  custom user agent (max 255 bytes)\n"
	         "         -w STRING  write this custom header to all requests (max 4095 bytes)\n"
	         "         -c         convert content to text format (with UTF-8 encoding)\n"
	         "         -8         convert from page encoding to UTF-8\n"
	         "         -DMILIS    set delay time in miliseconds when downloading more pages from the same IP (default is 100 ms)\n"
	         "         -S         disable SSL/TLS support\n"
	         "         -g         accept gzip encoding\n"
	         "         -6         resolve host to IPv6 address only\n"
	         "         -b STRING  cookies in the netscape/mozilla file format (max 20 cookies)\n"
	         "\n   urloptions:\n"
	         "         -C STRING  parameter which replaces '%%' in the custom header (max 255 bytes)\n"
	         "         -P STRING  HTTP POST parameters\n"
	         "         -X STRING  custom request HTTP method, no validation performed (max 15 bytes)\n"
	         "\n", VERSION);
}

/** a jedeeeem...
 */
int main(int argc, char *argv[]) {
	if(argc < 2) {
		printusage();
		exit(1);
	}
	
	signal(SIGUSR1,sighandler);
	signal(SIGPIPE,sighandler);
//	signal(SIGSEGV,sighandler);

	settings.timeout=DEFAULT_TIMEOUT;
	settings.delay=DEFAULT_DELAY;
	
	struct surl *url;
	url = initurls(argc, argv);
	go(url);
 
	exit(0);
}