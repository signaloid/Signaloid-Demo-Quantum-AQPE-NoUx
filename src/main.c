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
#include <stdio.h>
#include <stdlib.h>
#include <gsl/gsl_randist.h>
#include <sys/time.h>
#include "utilities.h"

void	initRNG(gsl_rng *  gslRNG);
double	calculateM(double standardDeviation, double alpha);
double	calculateTheta(double meanValue, double standardDeviation);
void	sampleFromRestrictedGaussian(double mu, double sigma, double *  samples, size_t  numberOfSamples, gsl_rng *  gslRNG);
void	runQPECircuit(double phi, uint64_t *  evidenceSampleCounts, uint64_t numberOfEvidenceSamples, gsl_rng *  gslRNG);
void	doRFPE(double *  priorSamples, size_t numberOfPriorSamples, uint64_t *  evidenceSampleCounts, uint64_t numberOfEvidenceSamples, double *  meanValue, double *  standardDeviation, gsl_rng *  gslRNG);

typedef enum
{
	kMaxNumberOfIterations = 100,
	kPosteriorStandardDeviationIncreaseFactor = 1,
} Constants;

double	currentM;
double	currentTheta;

void
initRNG(gsl_rng *  gslRNG)
{
	unsigned long	randomSeed = 0;

	if (randomSeed == 0)
	{
		/*
		 *	Set random seed from time of day.
		 */
		struct timeval	tv;
		gettimeofday(&tv, 0);
		randomSeed = ((tv.tv_sec>>10) ^ (tv.tv_usec<<10)) + 1;
	}
	fprintf(stderr, "Setting random seed to %lu.\n", randomSeed);
	gsl_rng_set(gslRNG, randomSeed);
}

double
calculateM(double standardDeviation, double alpha)
{
	if (standardDeviation == 0.0)
	{
		return 1.0;
	}
	else
	{
		return 1 / pow(standardDeviation, alpha);
	}
}

double
calculateTheta(double meanValue, double standardDeviation)
{
	return meanValue - standardDeviation;
}

void
sampleFromRestrictedGaussian(double mu, double sigma, double *  samples, size_t  numberOfSamples, gsl_rng *  gslRNG)
{
	double	gaussianSample;
	size_t	numberOfValidSamples = 0;

	while (numberOfValidSamples < numberOfSamples)
	{
		gaussianSample = gsl_ran_gaussian(gslRNG, sigma) + mu;

		if (fabs(gaussianSample) < M_PI)
		{
			samples[numberOfValidSamples] = gaussianSample;
			numberOfValidSamples++;
		}
	}

	return;
}

void
runQPECircuit(double phi, uint64_t *  evidenceSampleCounts, uint64_t numberOfEvidenceSamples, gsl_rng *  gslRNG)
{
	double		probabilityEvidence0GivenPhiPrior;
	double		uniformSample;
	uint64_t	i;

	probabilityEvidence0GivenPhiPrior = (1 + cos(currentM * (phi - currentTheta))) / 2;
	evidenceSampleCounts[0] = 0;

	for (i = 0; i < numberOfEvidenceSamples; i++)
	{
		uniformSample = gsl_ran_flat(gslRNG, 0.0, 1.0);
		if (uniformSample < probabilityEvidence0GivenPhiPrior)
		{
			evidenceSampleCounts[0]++;
		}
	}
	evidenceSampleCounts[1] = numberOfEvidenceSamples - evidenceSampleCounts[0];

	return;
}

void
doRFPE(double *  priorSamples, size_t numberOfPriorSamples, uint64_t *  evidenceSampleCounts, uint64_t numberOfEvidenceSamples, double *  meanValue, double *  standardDeviation, gsl_rng *  gslRNG)
{
	double		evidenceProbabilityGivenPriorSamples[numberOfPriorSamples];
	double		logEvidenceProbabilityGivenPriorSamples[numberOfPriorSamples];
	double		evidenceZeroProbabilityGivenPriorSamples[numberOfPriorSamples];
	double		maxOfLogEvidenceProbability;
	double		uniformSample;
	double		currentStandardDeviation = *standardDeviation;
	size_t		numberOfAcceptedPriorSamples = 0;
	size_t		i;
	
	for (i = 0; i < numberOfPriorSamples; i++)
	{
		evidenceZeroProbabilityGivenPriorSamples[i] = (1 + cos(currentM * (priorSamples[i] - currentTheta))) / 2;
		logEvidenceProbabilityGivenPriorSamples[i] = 0.0;
	}
	
	for (size_t k = 0; k < 2; k++)
	{
		maxOfLogEvidenceProbability = -INFINITY;
		
		for (i = 0; i < numberOfPriorSamples; i++)
		{
			if (k == 0)
			{
				logEvidenceProbabilityGivenPriorSamples[i] += log(evidenceZeroProbabilityGivenPriorSamples[i]) * evidenceSampleCounts[k];
			}
			else
			{
				logEvidenceProbabilityGivenPriorSamples[i] += log(1 - evidenceZeroProbabilityGivenPriorSamples[i]) * evidenceSampleCounts[k];
			}

			if (logEvidenceProbabilityGivenPriorSamples[i] > maxOfLogEvidenceProbability)
			{
				maxOfLogEvidenceProbability = logEvidenceProbabilityGivenPriorSamples[i];
			}
		}

		for (i = 0; i < numberOfPriorSamples; i++)
		{
			logEvidenceProbabilityGivenPriorSamples[i] -= maxOfLogEvidenceProbability;
		}
	}

	for (i = 0; i < numberOfPriorSamples; i++)
	{
		evidenceProbabilityGivenPriorSamples[i] = exp(logEvidenceProbabilityGivenPriorSamples[i]);
	}

	*meanValue = 0.0;
	*standardDeviation = 0.0;
	
	for (i = 0; i < numberOfPriorSamples; i++)
	{
		uniformSample = gsl_ran_flat(gslRNG, 0.0, 1.0);

		if (uniformSample <= evidenceProbabilityGivenPriorSamples[i])
		{
			numberOfAcceptedPriorSamples += 1;
			*meanValue += priorSamples[i];
			*standardDeviation += priorSamples[i] * priorSamples[i];
		}
	}

	if (numberOfAcceptedPriorSamples == 1)
	{
		*standardDeviation = currentStandardDeviation / 2;
	}
	else
	{
		*meanValue /= numberOfAcceptedPriorSamples;
		*standardDeviation = sqrt((*standardDeviation / numberOfAcceptedPriorSamples) - (*meanValue * *meanValue));
		*standardDeviation *= kPosteriorStandardDeviationIncreaseFactor;
	}

	return;
}

bool
runAQPEviaRFPEExperiment(double initialMeanValue, double initialStandardDeviation, CommandLineArguments *  arguments, size_t experimentNo, gsl_rng *  gslRNG, size_t *  convergenceIterationCount, double *  estimatedPhi)
{
	double *	priorSamples;
	uint64_t	evidenceSampleCounts[2];
	double		meanValue = initialMeanValue;
	double		standardDeviation = initialStandardDeviation;
	bool		convergenceAchieved  = false;
	size_t		i;
	
	/*
	 *	Allocate arrays.
	 */
	priorSamples = (double *) malloc(arguments->numberOfPriorTestSamplesPerIteration * sizeof(double));
	
	if (arguments->verbose)
	{
		printf("\nStarting AQPE Experiment #%zu:\n", experimentNo);
		printf("-------------------------------\n");
		printf("Iteration 0: Mean value of estimate Phi: %le,\tStandard deviation of estimate Phi: %le\n", meanValue, standardDeviation);
	}
	
	/*
	 *	Loop over RFPE iterations
	 */
	for (i = 0; i < kMaxNumberOfIterations; i++)
	{
		currentM = calculateM(standardDeviation, arguments->alpha);
		currentTheta = calculateTheta(meanValue, standardDeviation);
		
		runQPECircuit(arguments->targetPhi, evidenceSampleCounts, arguments->numberOfEvidenceSamplesPerIteration, gslRNG);
		sampleFromRestrictedGaussian(meanValue, standardDeviation, priorSamples, arguments->numberOfPriorTestSamplesPerIteration, gslRNG);
		doRFPE(priorSamples, arguments->numberOfPriorTestSamplesPerIteration, evidenceSampleCounts, arguments->numberOfEvidenceSamplesPerIteration, &meanValue, &standardDeviation, gslRNG);

		if (arguments->verbose)
		{
			printf("\nIteration %zu: Mean value of estimate Phi: %le,\tStandard deviation of estimate Phi: %le\n", i + 1, meanValue, standardDeviation);
		}

		/*
		 *	If the standard deviation of prior is smaller than precision, terminate.
		 */
		if (standardDeviation < arguments->precision)
		{
			*estimatedPhi = meanValue;
			*convergenceIterationCount = i + 1;
			convergenceAchieved = true;
			break;
		}
	}

	/*
	 *	Report the results of the current experiment.
	 */
	if (arguments->verbose)
	{
		if (convergenceAchieved)
		{
			printf("\nAQPE Experiment #%zu: Successfully acheieved precision in %zu iterative circuit mappings to quantum hardware! The final estimate has mean value %le and standard deviation %le.\n", experimentNo, i + 1, meanValue, standardDeviation);
		}
		else
		{
			printf("\nAQPE Experiment #%zu: Could not converge within the maximum allowed number of %d iterative circuit mappings to quantum hardware! The final estimate has mean value %le and standard deviation %le.\n", experimentNo, kMaxNumberOfIterations, meanValue, standardDeviation);
		}
	}

	/*
	 *	Free allocated arrays.
	 */
	free(priorSamples);

	return convergenceAchieved;
}

int
main(int argc, char *  argv[])
{
	CommandLineArguments	arguments = {
		.targetPhi				= M_PI / 2,
		.precision				= 1e-2,
		.alpha					= 1.0,
		.numberOfEvidenceSamplesPerIteration	= 0,
		.numberOfPriorTestSamplesPerIteration	= 1000,
		.numberOfRepetitions			= 1,
		.verbose				= false,
	};
	double			initialMeanValue = 0.0;
	double			initialStandardDeviation = M_PI / 2;
	double			estimatedPhi;
	size_t			convergenceIterationCount;
	double			averageNumberOfTotalIterations = 0.0;
	double			averageDistanceFromTarget = 0.0;
	size_t			wrongConvergenceCount = 0;
	size_t			convergenceCount = 0;
	double			xSigmaValue = 4.0;
	size_t			i;
	gsl_rng *		gslRNG;

	/*
	 *	Get command line arguments.
	 */
	if (getCommandLineArguments(argc, argv, &arguments))
	{
		return 1;
	}

	/*
	 *	Allocate a default GSL random number generator and intialize the RNG.
	 */
	gslRNG = gsl_rng_alloc(gsl_rng_default);
	initRNG(gslRNG);
	
	/*
	 *	Loop over AQPE experiments
	 */
	for (i = 0; i < arguments.numberOfRepetitions; i++)
	{
		/*
		 *	Run the AQPE (via RFPE) experiment and count converging experiments.
		 */
		if (runAQPEviaRFPEExperiment(initialMeanValue, initialStandardDeviation, &arguments, i + 1, gslRNG, &convergenceIterationCount, &estimatedPhi))
		{
			/*
			 *	Computing output variables of interest.
			 */
			averageNumberOfTotalIterations += (double) convergenceIterationCount;
			averageDistanceFromTarget += fabs(arguments.targetPhi - estimatedPhi);

			/*
			 *	Counting wrong-converging experiments.
			 */
			if (fabs(arguments.targetPhi - estimatedPhi) > xSigmaValue * arguments.precision)
			{
				wrongConvergenceCount++;
			}

			convergenceCount++;
		}
	}

	/*
	 *	Report results across all experiments.
	 */
	if (convergenceCount == 0)
	{
		printf("\nConvergence failed for all %zu AQPE experiments within the allowed maximum limit of %d iterative circuit mappings to quantum hardware!\n", arguments.numberOfRepetitions, kMaxNumberOfIterations);
	}
	else
	{
		averageNumberOfTotalIterations /= convergenceCount;
		averageDistanceFromTarget /= convergenceCount;

		printf("\nConvergence achieved on average in %lf iterative circuit mappings to quantum hardware in %zu of %zu AQPE experiments and yielded an average phase estimation error of %le.\n", averageNumberOfTotalIterations, convergenceCount, arguments.numberOfRepetitions, averageDistanceFromTarget);
		printf("\nIn %zu out of %zu converging experiments, the phase estimation error was greater than %d times the input precision %le.\n", wrongConvergenceCount, convergenceCount, (int) xSigmaValue, xSigmaValue * arguments.precision);
	}

	/*
	 *	Verbose mode reminder.
	 */
	if (!arguments.verbose)
	{
		printf("\nTo print details of all experiments, please run in verbose mode using the '-v' command-line argument option.\n");
	}
	
	/*
	 *	Free the allocated RNG.
	 */
	gsl_rng_free(gslRNG);

	return 0;
}
