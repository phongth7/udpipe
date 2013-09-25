/*****************************************************************************
Copyright 2013 Laboratory for Advanced Computing at the University of Chicago

This file is part of udtcat

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <getopt.h>
#include <iostream>

#include "udtcat.h"
#include "udtcat_server.h"
#include "udtcat_client.h"

using std::cerr;
using std::endl;

void usage(){
    fprintf(stderr, "usage: udtcat [udtcat options] host port\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "\t\t%s %s\t%s\n", "-l", "\t", "server");
    fprintf(stderr, "\t\t%s %s\t%s\n", "-n", "\t", "use encryption");
    fprintf(stderr, "\t\t%s %s\t%s\n", "-p", "password", "password string");
    fprintf(stderr, "\t\t%s %s\t%s\n", "-f", "path\t", "path to password");
    fprintf(stderr, "\t\t%s %s\t%s\n", "-v", "\t", "verbose");
    // fprintf(stderr, "\t\t%s %s\t%s\n", "", "", "");
    exit(1);
}

void initialize_thread_args(thread_args *args){
    args->ip = NULL;
    args->port = NULL;
    args->blast = 0;
    args->blast_rate = 1000;
    args->udt_buff = BUFF_SIZE;
    args->udp_buff = BUFF_SIZE;
    args->mss = 8400;
    args->use_crypto = 0;
    args->verbose = 0;
}

int main(int argc, char *argv[]){

    int opt;
    enum {NONE, SERVER, CLIENT};
    int operation = CLIENT;

    thread_args args;
    initialize_thread_args(&args);
    int use_crypto = 0;
    int verbose = 0;
    char* path_to_password = NULL;
    char* password = NULL;

    // ----------- [ Read in options
    while ((opt = getopt (argc, argv, "hvnlp:f:")) != -1){
	switch (opt){

	case 'l':
	    operation = SERVER;
	    break;

	case 'v':
	    args.verbose = 1;
	    break;

	case 'n':
	    args.use_crypto = 1;
	    use_crypto = 1;
	    break;

	case 'p':
	    password = strdup(optarg);
	    break;

	case 'f':
	    path_to_password = strdup(optarg);
	    break;

	case 'h':
	    usage();
	    break;

	default:
	    fprintf(stderr, "Unknown command line arg. -h for help.\n");
	    usage();
	    exit(1);

	}
    }

    if (use_crypto && (path_to_password && password)){
	fprintf(stderr, "error: Please specify either password or password file, not both.\n");
	exit(1);
    }

    if (path_to_password){
	FILE*password_file = fopen(path_to_password, "r");
	if (!password_file){
	    fprintf(stderr, "password file: %s.\n", strerror(errno));
	    exit(1);
	}

	fseek(password_file, 0, SEEK_END); 
	long size = ftell(password_file);
	fseek(password_file, 0, SEEK_SET); 
	password = (char*)malloc(size);
	fread(password, 1, size, password_file);
	
    }

    if (!use_crypto && password){
	fprintf(stderr, "warning: You've specified a password, but you don't have encryption turned on.\nProceeding without encryption.\n");
    }    

    if (use_crypto && !password){
	fprintf(stderr, "Please either: \n (1) %s\n (2) %s\n (3) %s\n",
		"include password in cli [-p password]",
		"read on in from file [-f /path/to/password/file]",
		"choose not to use encryption, remove [-n]");
	exit(1);
    }

    // Setup host
    if (operation == CLIENT){
	if (optind < argc){
	    args.ip = strdup(argv[optind++]);
	} else {
	    cerr << "error: Please specify server host." << endl;
	    exit(1);
	}
    }

    // Check port input
    if (optind < argc){
	args.port = strdup(argv[optind++]);
    } else {
	cerr << "error: Please specify port num." << endl;
	exit(1);
    }

    // Initialize crypto
    if (use_crypto){

	char* cipher = (char*) "aes-128";
	crypto enc(EVP_ENCRYPT, PASSPHRASE_SIZE, (unsigned char*)password, cipher);
	crypto dec(EVP_DECRYPT, PASSPHRASE_SIZE, (unsigned char*)password, cipher);
	memset(password, 0, strlen(password));
	args.enc = &enc;
	args.dec = &dec;

    } else {

	args.enc = NULL;
	args.dec = NULL;

    }



    // ----------- [ Spawn correct process
    if (operation == SERVER){
	run_server(&args);

    } else if (operation == CLIENT){
	run_client(&args);

    } else {
	cerr << "Operation type not known" << endl;
    
    }

    
  
}
