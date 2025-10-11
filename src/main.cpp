// otojsd - sound processing server for Otojs.

#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>

#include <string>
#include <vector>

#include "otojsd.h"
#include "const.h"

const char options_short[] = "p:fvc:r:a:o:id:l";
const struct option options_long[] = {
	{ "port"   , required_argument, NULL, 'p' },
	{ "findfreeport",  no_argument, NULL, 'f' },
	{ "verbose",       no_argument, NULL, 'v' },
	{ "channel", required_argument, NULL, 'c' },
	{ "rate"   , required_argument, NULL, 'r' },
	{ "allow"  , required_argument, NULL, 'a' },
	{ "output" , required_argument, NULL, 'o' },
	{ "enable-input",  no_argument, NULL, 'i' },
	{ "document-root", required_argument, NULL, 'd' },
	{ "level-meter"  , no_argument, NULL, 'l' },
};

char errortext[256];

// -------------------------------------------- private functions
long options_integer(const char *arg, long min, long max, const char *text);
void die(const char *text);

// -------------------------------------------- main
int main(int argc, char **argv, char **env) {
	otojsd_options options = OTOJSD_OPTIONS_DEFAULTS;

	int result, length;
	while( (result = getopt_long(argc, argv, options_short, options_long, NULL)) != -1 ){
		switch(result){
			case 'p':
				options.port = options_integer(optarg, 0, 65535, "-p, --port");
				break;
			case 'f':
				options.findfreeport = true;
				break;
			case 'v':
				options.verbose = true;
				break;
			case 'c':
				options.channel = options_integer(optarg, 1, 128, "-c, --channel");
				break;
			case 'r':
				options.sample_rate = options_integer(optarg, 1, 192000, "-r, --rate");
				break;
			case 'a':
				length = strlen(optarg);
				if ( length <= 0 || 39 < length)
					die("-a, --allow parameter is like '192.168.1.3' or '192.168.2.0/255.255.255.0'.");
				options.allow_pattern = optarg;
				break;
			case 'o':
				options.output = optarg;
				break;
			case 'i':
				options.enable_input = true;
				break;
			case 'd':
				options.document_root = optarg;
				break;
			case 'l':
				options.level_meter = true;
				break;
		}
	}

	std::vector<std::string> start_codes;
	if (optind < argc) {
		for (int i = optind; i < argc; i++) {
			start_codes.push_back(argv[i]);
		}
	} else {
		start_codes.push_back(OTOJSD_DEFAULT_STARTCODE);
	}

	otojsd_start(&options, start_codes, argv[0], env);

	return 0;
}


// ----------------------------------------------- functions
long options_integer(const char *arg, long min, long max, const char *text) {
	char *err;
	long number = strtol(arg, &err, 0);
	
	if (*err != '\0') {
		snprintf(errortext, 256, "%s parameter must be a number.", text);
		die(errortext);
	}else if( number < min || max < number ) {
		snprintf(errortext, 256, "%s parameter must be in %ld - %ld.", text, min, max);
		die(errortext);
	}
	return number;
}

void die(const char *text) {
	if (text) perror(text);
	exit(-1);
}
