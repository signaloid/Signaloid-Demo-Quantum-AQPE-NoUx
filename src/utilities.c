/*
 *	Authored 2023, Bilgesu Bilgin.
 *
 *	Copyright (c) 2023, Signaloid.
 *
 *	All rights reserved.
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions
 *	are met:
 *	*	Redistributions of source code must retain the above
 *		copyright notice, this list of conditions and the following
 *		disclaimer.
 *	*	Redistributions in binary form must reproduce the above
 *		copyright notice, this list of conditions and the following
 *		disclaimer in the documentation and/or other materials
 *		provided with the distribution.
 *	*	Neither the name of the author nor the names of its
 *		contributors may be used to endorse or promote products
 *		derived from this software without specific prior written
 *		permission.
 *
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *	POSSIBILITY OF SUCH DAMAGE.
 */
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "utilities.h"

const double	kMinimumAlpha = 0.0;
const double	kMaximumAlpha = 1.0;
const double	kMinimumPhi = -M_PI;
const double	kMaximumPhi = M_PI;
const double	kMinimumPrecision = 1e-10;
const double	kMaximumPrecision = 1.0;
const uint64_t	kMaximumNumberOfEvidenceSamples = 1000000;

/**
 *	@brief	Print out command line usage.
 */
void
printUsage(void)
{
	fprintf(stdout, "\nExample: Accelerated Quantum Phase Estimation (AQPE) using Rejection Filtering Phase Estimation (RFPE)\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "Command line arguments:\n");
	fprintf(stdout,
		
		"[-t <target_phase : double in [-pi, pi]>] (Default: pi / 2)\n"
		"[-p <precision_in_phase_estimation : double in [%le, %le]>] (Default: 1e-4)\n"
		"[-a <alpha : double in [0,1]>] (Default: 0.5)\n"
		"[-n <number_of_evidence_samples_per_iteration : int in [0, inf)>] (Default: see README.md)\n"
		"[-m <number_of_prior_test_samples_per_iteration : int in (0, inf)>] (Default: 1000)\n"
		"[-r <number_of_repetitions : size_t in (0, inf)>] (Default: 1)\n"
		"[-v] (Verbose mode: Prints details of each repeated AQPE experiment to stdout.)\n"
		"[-h] (Display this help message.)\n", kMinimumPrecision, kMaximumPrecision);
	fprintf(stdout, "\n");
}

/**
 *	@brief	Get command line arguments.
 *
 *	@param	argc		: argument count from main()
 *	@param	argv		: argument vector from main()
 *	@param	arguments	: Pointer to struct to store arguments
 *	@return	int		: 0 if successful, else 1
 */
int
getCommandLineArguments(int argc, char *  argv[], CommandLineArguments * arguments)
{
	int	opt;
	bool	userSpecifiedEvidenceNumber = false;

	opterr = 0;

	while ((opt = getopt(argc, argv, ":t:p:a:n:m:r:vh")) != EOF)
	{
		switch (opt)
		{
			case 't':
			{
				if ((atof(optarg) < kMinimumPhi) || (atof(optarg) > kMaximumPhi))
				{
					fprintf(stderr, "\nWarning: The argument of option -%c (precision) should be in [%le, %le]. Continuing with the default value %le.\n", opt, kMinimumPrecision, kMaximumPrecision, arguments->targetPhi);
				}
				else
				{
					arguments->targetPhi = atof(optarg);
				}
				break;
			}
			case 'p':
			{
				if ((atof(optarg) < kMinimumPrecision) || (atof(optarg) > kMaximumPrecision))
				{
					fprintf(stderr, "\nWarning: The argument of option -%c (precision) should be in [%le, %le]. Continuing with the default value %le.\n", opt, kMinimumPrecision, kMaximumPrecision, arguments->precision);
				}
				else
				{
					arguments->precision = atof(optarg);
				}
				break;
			}
			case 'a':
			{
				if ((atof(optarg) < kMinimumAlpha) || (atof(optarg) > kMaximumAlpha))
				{
					fprintf(stderr, "\nWarning: The argument of option -%c (precision) should be in [%le, %le]. Continuing with the default value %le.\n", opt, kMinimumAlpha, kMaximumAlpha, arguments->alpha);
				}
				else
				{
					arguments->alpha = atof(optarg);
				}
				break;
			}
			case 'n':
			{
				if (atoi(optarg) < 0)
				{
					fprintf(stderr, "\nError: The argument of option -%c (number of samples per Bayesian inference iteration) should be a non-negaitive integer. Use '-%c 0' to trigger automatic selection.\n", opt, opt);

					return 1;
				}
				else
				{
					arguments->numberOfEvidenceSamplesPerIteration = (uint64_t) atoi(optarg);
					userSpecifiedEvidenceNumber = true;
				}

				break;
			}
			case 'm':
			{
				arguments->numberOfPriorTestSamplesPerIteration = atoi(optarg);
				
				if (arguments->numberOfPriorTestSamplesPerIteration <= 0)
				{
					fprintf(stderr, "\nError: The argument of option -%c (number of prior test samples per Bayesian inference iteration) should be a positive integer.\n", opt);

					return 1;
				}

				break;
			}
			case 'r':
			{
				arguments->numberOfRepetitions = atoi(optarg);
				
				if (arguments->numberOfRepetitions <= 0)
				{
					fprintf(stderr, "\nError: The argument of option -%c (number of repetitions of the AQPE experiment) should be a positive integer.\n", opt);

					return 1;
				}

				break;
			}
			case 'v':
			{
				arguments->verbose = true;
				break;
			}
			case 'h':
			{
				printUsage();
				exit(0);
			}
			case ':':
			{
				fprintf(stderr, "\nError: Option -%c is missing a required argument.\n", optopt);
				printUsage();
				
				return 1;
				break;
			}
			case '?':
			{
				fprintf(stderr, "\nError: Invalid option: -%c.\n", optopt);
				printUsage();
				
				return 1;
			}
		}
	}

	if (arguments->numberOfEvidenceSamplesPerIteration == 0)
	{
		if (arguments->alpha == 1.0)
		{
			arguments->numberOfEvidenceSamplesPerIteration = (uint64_t) ceil(4 * log(1 / arguments->precision));
		}
		else
		{
			arguments->numberOfEvidenceSamplesPerIteration = (uint64_t) ceil((2 / (1 - arguments->alpha)) * (1 / pow(arguments->precision, 2 * (1 - arguments->alpha)) - 1));
		}
		
		if ((!userSpecifiedEvidenceNumber) && (arguments->numberOfEvidenceSamplesPerIteration > kMaximumNumberOfEvidenceSamples))
		{
			fprintf(stderr, "\nWarning: The number of samples required from the quantum circuit, N = %"PRIu64", has exceeded the allowed maximum limit of %"PRIu64" samples. Using the maximum allowed.\n", arguments->numberOfEvidenceSamplesPerIteration, kMaximumNumberOfEvidenceSamples);
			fprintf(stderr, "Note: Use '-n 0' to permit the use of high default number of samples. You can also specify custom number of samples by using the '-n' command-line argument option, e.g., '-n %"PRIu64"'.\n", 10 * kMaximumNumberOfEvidenceSamples);
			arguments->numberOfEvidenceSamplesPerIteration = kMaximumNumberOfEvidenceSamples;
		}
	}

	if (arguments->verbose)
	{
		printf("\nIn verbose mode!\n");
	}

	printf("targetPhi = %lf\n", arguments->targetPhi);
	printf("alpha = %lf\n", arguments->alpha);
	printf("precision = %le\n", arguments->precision);
	printf("numberOfEvidenceSamplesPerIteration = %"PRIu64"\n", arguments->numberOfEvidenceSamplesPerIteration);
	printf("numberOfPriorTestSamplesPerIteration = %zu\n", arguments->numberOfPriorTestSamplesPerIteration);
	printf("numberOfRepetitions = %zu\n", arguments->numberOfRepetitions);
	printf("\nRequired Quantum Circuit Depth = 1 / precision^{alpha} = %"PRIu64"\n", (uint64_t) ceil(1 / pow(arguments->precision, arguments->alpha)));
	printf("\nRequired Quantum Circuit Samples (N) = %"PRIu64"\n", (arguments->precision == 1.0) ? (uint64_t) ceil(4 * log(1 / arguments->precision)) : (int) ceil((2 / (1 - arguments->alpha)) * (1 / pow(arguments->precision, 2 * (1 - arguments->alpha)) - 1)));

	return 0;
}
