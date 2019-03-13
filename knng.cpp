/*******************************************************************************
 *
 * This file is part of RKNNG software.
 * Copyright (C) 2015-2018 Sami Sieranoja
 * <samisi@uef.fi>, <sami.sieranoja@gmail.com>
 *
 * RKNNG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version. You should have received a copy
 * of the GNU Lesser General Public License along with RKNNG.
 * If not, see <http://www.gnu.org/licenses/lgpl.html>.
 *******************************************************************************/

#include <stdio.h>
#include "contrib/argtable3.h"

#include "timer.h"
#include "util.h"
#include "globals.h"

#include "dataset.h"
#include "knngraph.h"

kNNGraph* g_ground_truth;
#include "recall.h"

#include "rp_div.h"


int main (int argc, char *argv[]) {

    struct arg_file * infn;
    struct arg_file * outfn;
    struct arg_file * gtfn;
    struct arg_dbl * stopDelta;
    struct arg_dbl * distpar;
    struct arg_str * outf;
    struct arg_str * informat;
    struct arg_str * algo;
    struct arg_str * dtype;
    struct arg_str * distfunc;
    struct arg_int * numNeighbors;
    struct arg_int * rngSeed;
    struct arg_lit * help;
    struct arg_end *end;

    DataSet* DS=NULL;
    kNNGraph* kNN;
    //kNNGraph* ground_truth = NULL;
    g_ground_truth = NULL;



    void *argtable[] = {
        help    = arg_litn(NULL, "help", 0, 1, "display this help and exit"),

        stopDelta   = arg_dbln(NULL, "delta", "<STOP>", 0, 1, "Stop when delta < STOP "),
        distpar   = arg_dbln(NULL, "distpar", "<FLOAT>", 0, 1, "Parameter to distance function (minkowski p-value)"),
        numNeighbors = arg_intn("k", "num_neighbors", "<n>", 0, 1, "number of neighbors"),
        rngSeed = arg_intn(NULL, "seed", "<n>", 0, 1, "random number seed"),

        dtype = arg_str0(NULL,"type","<vec|txt>","Input data type: vectorial or text."),
        distfunc = arg_str0(NULL,"dfunc","<FUNC>","Distance function:\n"
                "     l2 = euclidean distance (vectorial, default)\n"
                "     mnkw = Minkowski distance (vectorial)\n"
                "     lev = Levenshtein distance (for strings, default)\n"
                "     dice = Dice coefficient / bigrams (for strings)\n"
                ),
        informat = arg_str0(NULL,"format","<ascii|lshkit>","Input format: ascii or lshkit (binary)"),
        outf = arg_str0(NULL,"outf","<format>","Output format: {txt,ivec,wgraph}"),
        algo = arg_str0(NULL,"algo","<name>","Algorithm: {rpdiv,nndes}"),

        outfn    = arg_filen("o", "out", "<file>", 0, 1, "output file"),
        gtfn    = arg_filen(NULL, "gt", "<file>", 0, 1, "Ground truth graph file (ivec)"),
        infn    = arg_filen(NULL, NULL, "<file>", 1, 1, "input files"),

        end     = arg_end(20),
    };


    int ok=1;
    int nerrors = arg_parse(argc,argv,argtable);
    //if(nerrors > 0) {terminal_error("Unable to parse command line\n");}
    if(nerrors > 0) {ok=0;}

    double delta=0.01;
    int K=20;
    int W;

    if(stopDelta->count > 0) {
        delta = stopDelta->dval[0];
    }
    if(rngSeed->count > 0) {
        printf("Set RNG seed: %d\n",rngSeed->ival[0]);
        srand(rngSeed->ival[0]);
    }
    else {
        srand (time(NULL));
    }


    if(numNeighbors->count > 0) {
        K = numNeighbors->ival[0];
    }
    W=2.5*K;

    if(outf->count > 0 && strcmp(outf->sval[0],"txt")==0) {
        printf("Output format:txt\n");
    }

    if(infn->count > 0) {
    }
    else {ok=0;}


    if (help->count > 0 || ok==0)
    {
        printf("kNN-graph construction by Hierarchical Random Pair Division (v. 0.1).\n\nrknng");
        arg_print_syntax(stdout, argtable, "\n");
        arg_print_glossary(stdout, argtable, "  %-25s %s\n");
        return 0;
    }

    printf("K=%d delta=%f infn='%s'\n",K,delta,infn->filename[0]);

    if(dtype->count > 0 && strcmp(dtype->sval[0],"txt")==0) {
        printf("Loading (string) dataset: %s\n", infn->filename[0]);
        DS = loadStringData(infn->filename[0]);
        debugStringDataset(DS);
    }
    else if(dtype->count > 0 && strcmp(dtype->sval[0],"vec")==0) {
        if(1) {
            printf("Loading (vectorial) dataset in ascii format: %s\n", infn->filename[0]);
            DS = read_ascii_dataset(infn->filename[0]);
        }
        else {
            printf("Loading (vectorial) dataset in lshkit format: %s\n", infn->filename[0]);
            DS = read_lshkit(infn->filename[0]);
        }
    }
    else {
        terminal_error("--type not specified");
    }

    if(distpar->count > 0) {
        g_options.minkowski_p=distpar->dval[0];
    }
    else {
        g_options.minkowski_p=1.0; // L1 = Manhattan distance
    }


    g_options.distance_type = 0;
    if(distfunc->count > 0 && strcmp(distfunc->sval[0],"l2")==0) {
        printf("Distance function: %s\n",distfunc->sval[0]);
        g_options.distance_type = 0;
    }
    else if(distfunc->count > 0 && strcmp(distfunc->sval[0],"mnkw")==0) {
        g_options.distance_type = 1;
        printf("Distance function: minkowski (p=%f)\n",g_options.minkowski_p);
    }

    if(gtfn->count > 0) {
        printf("Loading ground truth file: %s\n", gtfn->filename[0]);
        g_ground_truth = load_kNN_ivec(gtfn->filename[0], RANDOM_SAMPLED_BRUTEFORCE);
        recalc_dist(g_ground_truth,DS);
    }

    g_options.recall_K = K;

    // Start counting time
    g_timer.tick();

    // Construct by NNDES
    if(algo->count > 0 && strcmp(algo->sval[0],"nndes")==0) {
        printf("Algorithm: NNDES\n");
        kNN = rpdiv_create_knng(DS, DS, K, 0,delta,0.0,100);
    }
    // Construct by RPDIV
    else {
        printf("Algorithm: RPDIV\n");
        kNN = rpdiv_create_knng(DS, DS, K, W,delta,0.1,100);
    }

    if(kNN && outfn->count > 0 ) {
        // binary format
        if(outf->count > 0 && strcmp(outf->sval[0],"bin")==0) {
            write_kNN_ivec(outfn->filename[0],kNN,0);
        }
        if(outf->count > 0 && strcmp(outf->sval[0],"wgraph")==0
                && DS->type==2) {
            //Only for string data
            write_string_graph(outfn->filename[0],kNN,DS);
        }
        // txt format
        else {
            write_kNN_txt(outfn->filename[0],kNN);
        }
    }

    free_DataSet(DS);
    free_kNNGraph(kNN);

}

