/*
 * Copyright [2018] [XiaoFei Zhao]
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BH_COMMANDS_HPP
#define BH_COMMANDS_HPP

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

void allusage(const int argc, const char *const *argv) {
	std::cerr << "Usage:\n  " << argv[0] << " <commmand> [options] [arguments ...]\n\n";
	std::cerr << "Commands:\n\n";
	std::cerr << "  sketch: reduce mutiple genomes into one sketch.\n" 
	          << "    A genome corresponds to a input sequence file.\n" 
	          << "    A sketch consists of a set of output files.\n\n";
	std::cerr << "  dist: estimate distance (and relevant statistics) between\n"
	          << "    genomes in query sketch and genomes in target-sketch.\n" 
	          << "    Query and target sketches are generated by the sketch command.\n\n";
	std::cerr << "  exact: estimate distance (and relevant statistics) between\n"
	          << "    genomes corresponding to input files.\n\n";
	std::cerr << "Notes:\n\n";
	std::cerr << "  To see command-specific usage, please enter\n"
	          << "    " << argv[0] << " command --help\n\n";
	std::cerr << "  The format for options is --NAME=VALUE\n\n";	
	exit(-2);
}

class Sketchargs {
public:
	std::vector<std::string> infnames;
	size_t bbits = 14; // 2; // = log2(64 * sketchsize64) TODO: find theoretical justification
	bool iscasepreserved = false;
	bool isstrandpreserved = false;
	std::string listfname = "-";
	size_t kmerlen = 21;
	int minhashtype = 2;
	size_t nthreads = 1;
	std::string outfname = "";
	uint64_t randseed = 41; //0x3355557799AACCULL;
	size_t sketchsize64 = 32;
	Sketchargs() : outfname("sketch-at-time-" + std::to_string(time(NULL))) {}
	int usage(const int argc, const char *const *argv);
	int write(const std::string &systime_began, const std::string &systime_ended, size_t n_cpu_seconds);
	int parse(const int argc, const char *const *argv);
	int read(std::string fname);
private:
	int _parse(std::string arg);
};

template <class T>
bool areequals_printerrifnot(const T a, const T b, const std::string &varname, std::string &infname1, std::string &infname2) {
	if (a != b) {
		std::cerr << "Error: " << varname << " has values " << a << " and " << b 
		          << " in files " << infname1 << " and " << infname2 <<  ", respectively\n";
		return false;
	}
	return true;
}

void verifycompatible(const Sketchargs &args1, const Sketchargs &args2, std::string &infname1, std::string &infname2) {
	bool isok = true;
	isok &= areequals_printerrifnot(args1.bbits, args2.bbits, "--bbits", infname1, infname2);
	isok &= areequals_printerrifnot(args1.iscasepreserved, args2.iscasepreserved, "--iscasepreserved", infname1, infname2);
	isok &= areequals_printerrifnot(args1.isstrandpreserved, args2.isstrandpreserved, "--isstrandpreserved", infname1, infname2);
	isok &= areequals_printerrifnot(args1.kmerlen, args2.kmerlen, "--kmerlen", infname1, infname2);
	isok &= areequals_printerrifnot(args1.minhashtype, args2.minhashtype, "--minhashtype", infname1, infname2);
	isok &= areequals_printerrifnot(args1.randseed, args2.randseed, "--randseed", infname1, infname2);
	isok &= areequals_printerrifnot(args1.sketchsize64, args2.sketchsize64, "--sketchsize64", infname1, infname2);
	if (!isok) { exit(2); }
}

int Sketchargs::usage(const int argc, const char *const *argv) {
	assert(1 < argc);
	std::cerr << "Usage: " << argv[0] << " " << argv[1] << " [options] [arguments ...]" << "\n\n";
	std::cerr << "Arguments:\n\n"
	          << "  Zero or more filenames. If zero filenames, then read from each line in listfname.\n"
	          << "  Each filename specifies a path to a sequence file.\n\n";
	std::cerr << "Options with [default values]:\n\n";
	std::cerr << "  --help : Show this help message." << "\n\n";
	std::cerr << "  --listfname: Name of the file associating consecutive sequences to genomes (including metagenomes and pangenomes).\n"
	          << "    Each line of this file has the following format:\n"
	          << "    \"Path-to-a-sequence-file(F) <TAB> [genome-name(G) <TAB> number-of-consecutive-sequences(N) ...]\".\n"
	          << "    If only F is provided, then use F as G and let N be the number of sequences in N [" << listfname << "]\n\n";
	std::cerr << "  --nthreads : This many threads will be spawned for processing. [" << nthreads << "]\n\n";
	std::cerr << "  --minhashtype : Type of minhash.\n" 
	          << "    -1 means perfect hash function for nucleotides where 5^(kmerlen) < 2^63.\n" 
	          << "    0 means one hash-function with multiple min-values.\n"
	          << "    1 means multiple hash-functions and one min-value per function.\n"
	          << "    2 means one hash-function with partitionned buckets. [" << minhashtype << "]\n\n";
	std::cerr << "  --bbits : Number of bits kept as in b-bits minhash. [" << bbits << "]\n\n";
	std::cerr << "  --kmerlen : K-mer length used to generate minhash values. [" << kmerlen << "]\n\n";
	std::cerr << "  --sketchsize64 : Sketch size divided by 64, or equivalently,\n" 
	          << "    the number of sets (each consisting of 64 minhash values) per genome). [" << sketchsize64 << "]\n\n";
	std::cerr << "  --isstrandpreserved : Preserve strand, which means ignore reverse complement. [" << std::boolalpha << isstrandpreserved << "]\n\n";
	std::cerr << "  --iscasepreserved : Preserve case, which means the lowercase and uppercase versions of the\n" 
	          << "    same letter are treated as two different letters. [" << std::boolalpha << iscasepreserved << "]\n\n";
	std::cerr << "  --randseed : Seed to provide to the hash function. [" << randseed << "]." << "\n\n";
	std::cerr << "  --outfname : Name of the file containing sketches as output [" << outfname << " (time-dependent)]." << "\n\n";
	std::cerr << "Notes:\n\n";
	std::cerr << "  \"-\" (without quotes) means stdin.\n";
	std::cerr << "  For general usage, please enter\n"
	          << "    " << argv[0] << " --help\n\n";
	std::cerr << "  The following is an example of options: --nthreads=8\n\n";
	exit(1);
}

int Sketchargs::write(const std::string &systime_began, const std::string &systime_ended, size_t n_cpu_seconds) {
	std::ofstream file(outfname, std::ios::out);
	if (!file) {
		std::cerr << "Error: cannot open the file " << outfname << " for writing\n";
		exit(-1);
	}
	file << "--COMMENT=program began at " << systime_began; // << "\n"; // no new line
	file << "--COMMENT=program ended at " << systime_ended; // << "\n"; // no new line
	file << "--COMMENT=program consumed " << n_cpu_seconds << " CPU seconds\n"; // << "\n"; // no new line
	file << "--COMMENT=revision " << (STR(GIT_COMMIT_HASH)) << " " << (STR((GIT_DIFF_SHORTSTAT))) << "\n";
	file << "--bbits=" << bbits << "\n";
	file << std::boolalpha << "--iscasepreserved=" << iscasepreserved << "\n";
	file << std::boolalpha << "--isstrandpreserved=" << isstrandpreserved << "\n";
	file << "--kmerlen=" << kmerlen << "\n";
	file << "--listfname=" << listfname << "\n";
	file << "--minhashtype=" << minhashtype << "\n";
	file << "--nthreads=" << nthreads << "\n";
	file << "--outfname=" << outfname << "\n";
	file << "--randseed=" << randseed << "\n";
	file << "--sketchsize64=" << sketchsize64 << "\n";
	file.close();
	return 0;
}

int Sketchargs::read(std::string fname) {
	std::ifstream file(fname, std::ios::in);
	if (!file) {
		std::cerr << "Error: cannot open the file " << fname << " for reading\n";
		exit(-1);
	}
	std::string line;	
	std::getline(file, line);
	while (std::getline(file, line)) {
		int result = _parse(line);
		if (result != 0) {
			std::cerr << "The line " << line << " in the file " << fname << " is invalid.\n";
			exit(2);
		}
	}
	file.close();
	return 0;
}

int Sketchargs::parse(const int argc, const char *const *argv) {
	for (int i = 2; i < argc; i++) {
		std::string arg(argv[i]);
		int result = _parse(arg);
		if (-1 == result) {
			std::cerr << "Unrecognized option: " << arg << "\n\n";
			usage(argc, argv);
		} else if (-2 == result) {
			std::cerr << "Option without value: " << arg << "\n\n";
			usage(argc, argv);
		} else if (1 == result) {
			usage(argc, argv);
		}
	}
	if (infnames.size() == 0 && listfname == "-") {
		std::cerr << "Reading input filenames from stdin.\n";
		std::cerr << "Enter \"--help\" (without quotes) to show usage.\n";
		for (std::string line; std::getline(std::cin, line);) {
			if (line == "--help") { usage(argc, argv); }
			infnames.push_back(line);
		}
	} else if (infnames.size() == 0) {
		std::cerr << "Reading input filenames from " << listfname << ".\n";
		std::ifstream listfstream(listfname);
		if (!listfstream) {
			std::cerr << "The listfname "  << listfname << " cannot be opened.\n";
			exit(4);
		}
		for (std::string line; std::getline(listfstream, line);) {
			infnames.push_back(line);
		}
		listfstream.close();
	}
	return 0;
}

int Sketchargs::_parse(std::string arg) {
	std::istringstream iss(arg);
	std::string key;
	std::string val;
	std::getline(iss, key, '=');
	std::getline(iss, val);
	std::istringstream issval(val);
	if (arg.find("--help", 0) == 0) { return 1; }
	else if ("--bbits" == key) { issval >> bbits; }
	else if ("--iscasepreserved" == key) { issval >> std::boolalpha >> iscasepreserved; }
	else if ("--isstrandpreserved" == key) { issval >> std::boolalpha >> isstrandpreserved; }
	else if ("--kmerlen" ==  key) { issval >> kmerlen; }
	else if ("--listfname" ==  key) { issval >> listfname; }
	else if ("--minhashtype" ==  key) { issval >> minhashtype; }
	else if ("--nthreads" == key) { issval >> nthreads; }
	else if ("--outfname" == key) { issval >> outfname; }
	else if ("--randseed" == key) { issval >> randseed; }
	else if ("--sketchsize64" == key) { issval >> sketchsize64; }
	else if ("--COMMENT" == key) { /* do nothing */ }
	else if (key.find("--", 0) != 0) {
		infnames.push_back(arg);
		return 0; 
	} else {
		return -1;
	}
	if (issval.fail()) {
		return -2;
	}
	return 0;
}

void parse_metaf(std::vector<std::vector<std::pair<size_t, size_t>>> &fid_to_entityid_count_list,
		std::vector<std::string> &fid_to_fname,
		std::vector<std::string> &entityid_to_entityname,
		const std::vector<std::string> &infnames) {
	
	assert(0 == fid_to_entityid_count_list.size());
	assert(0 == fid_to_fname.size());
	assert(0 == entityid_to_entityname.size());
	
	std::set<std::string> entitynames;
	for (size_t i = 0; i < infnames.size(); i++) {
		std::istringstream iss(infnames[i]);
		std::string fname;
		std::getline(iss, fname, '\t');
		assert(!iss.fail());
		fid_to_fname.push_back(fname); // 
		fid_to_entityid_count_list.push_back(std::vector<std::pair<size_t, size_t>>()); // 
		std::string entityname;
		size_t nseqs;
		while (std::getline(iss, entityname, '\t')) {
			std::string nseqs_str;
			std::getline(iss, nseqs_str, '\t');
			std::istringstream iss2(nseqs_str);
			iss2 >> nseqs;
			assert(nseqs > 0);
			assert(iss.fail() == iss2.fail());
			// std::cerr << "Entity " << entityname << " is inserted." << std::endl;
			auto res = entitynames.insert(entityname);
			if (!res.second) {
				std::cerr << "The genome " << entityname << " is duplicated." << std::endl;
				exit(-8);
			}
			auto entityid = entityid_to_entityname.size(); 	
			entityid_to_entityname.push_back(entityname);
			fid_to_entityid_count_list[i].push_back(std::make_pair(entityid, nseqs));
		}
		if (fid_to_entityid_count_list[i].size() == 0) {
			entityname = fname;
			auto res = entitynames.insert(entityname);
			if (!res.second) {
				std::cerr << "The genome and the file " << entityname << " is duplicated." << std::endl;
				exit(-16);
			}
			auto entityid = entityid_to_entityname.size();
			entityid_to_entityname.push_back(entityname);
			fid_to_entityid_count_list[i].push_back(std::make_pair(entityid, SIZE_MAX - 1));
		}
	}
	assert(fid_to_entityid_count_list.size() == infnames.size());
}

class Distargs {
public:
	std::vector<std::string> infnames;
	size_t ithres = 2;
	double mthres = 2.5;
	size_t nneighbors = 0;
	size_t nthreads = 1;
	std::string outfname = "-";
	double pthres = 1 + 1e-4;
	int usage(const int argc, const char *const *argv);
	int parse(const int argc, const char *const *argv);
private:
	int _parse(std::string arg);
};

int Distargs::usage(const int argc, const char *const *argv) {
	assert(1 < argc);
	std::cerr << "Usage: " << argv[0] << " " << argv[1] << " [options] query-sketch [target-sketch]\n\n";
	std::cerr << "  Query-sketch and target-sketch: sketches used as query and target.\n" 
	          << "    Sketches are generated by \"" << argv[0] << " sketch\" (without quotes).\n" 
	          << "    If target-sketch is omitted, then query-sketch is used as both query and target.\n\n";
	std::cerr << "Options:\n\n";
	std::cerr << "  --help : Show this help message." << "\n\n";
	std::cerr << "  --ithres : If intersection(A, B) has less than this number of elements,"
	          << "    then set the intersection to empty set so that the resulting Jaccard-index is zero. [" << ithres << "]\n\n";
	std::cerr << "  --mthres : Only results with at most this mutation distance are reported [" << mthres << "]\n\n";
	std::cerr << "  --nneighbors : Only this number of best-hit results per query are reported.\n" 
	          << "    If this value is zero then report all. [" << nneighbors << "].\n\n";
	std::cerr << "  --nthreads : This many threads will be spawned for processing. [" << nthreads << "]\n\n";
	std::cerr << "  --outfname : The output file comparing the query and target sketches.\n"
	          << "    The ouput file contains the following tab-separated fields per result line:\n"
	          << "    query-sketch, target-sketch, mutation-distance, p-value, and jaccard-index. [" << outfname << "].\n\n";
	std::cerr << "  --pthres : only results with at most this p-value are reported. [" << pthres << "]\n\n";
	std::cerr << "Note:\n\n";
	std::cerr << "  If target-sketch is omitted and --nneighbors is zero,\n" 
	          << "    then distance from genome A to genome B is the same as distance from B to A.\n"
	          << "    In this case, only one record is reported per set of two genomes due to reflectivity.\n\n";
	std::cerr << "  \"-\" (without quotes) means stdout.\n\n";
	std::cerr << "  For general usage, please enter\n"
	          << "    " << argv[0] << " --help\n\n";
	std::cerr << "  The following is an example of options: --mthres=0.2\n\n";
	exit(1);
}

int Distargs::_parse(std::string arg) {
	std::istringstream iss(arg);
	std::string key;
	std::string val;
	std::getline(iss, key, '=');
	std::getline(iss, val);
	std::istringstream issval(val);
	if (arg.find("--help", 0) == 0) { return 1; }
	else if ("--ithres" == key) { issval >> ithres; }
	else if ("--mthres" == key) { issval >> mthres; }
	else if ("--nneighbors" == key) { issval >> nneighbors; }
	else if ("--nthreads" ==  key) { issval >> nthreads; }
	else if ("--outfname" == key) { issval >> outfname; }
	else if ("--pthres" ==  key) { issval >> pthres; }
	else if (key.find("--", 0) != 0) {
		infnames.push_back(arg);
		return 0;
	} else {
		return -1;
	}
	if (issval.fail()) {
		return -2;
	}
	return 0;
}

int Distargs::parse(const int argc, const char *const *argv) {
	for (int i = 2; i < argc; i++) {
		std::string arg(argv[i]);
		int result = _parse(arg);
		if (-1 == result) {
			std::cerr << "Unrecognized option: " << arg << "\n\n";
			usage(argc, argv);
		} else if (-2 == result) {
			std::cerr << "Option without value: " << arg << "\n\n";
			usage(argc, argv);
		} else if (1 == result) {
			usage(argc, argv);
		}
	}
	if (infnames.size() != 1 && infnames.size() != 2) {
		std::cerr << "Either one or two non-optional arguments are required.\n";
		usage(argc, argv);
	}
	return 0;
}

#endif
