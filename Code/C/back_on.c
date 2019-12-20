#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

//-------------------------Begin Definitions-------------------------//
	#define turn_angle  2.*M_PI
//Main parameters
	#define N 50	 //Number of Kuramoto oscillators
	#define n_runs 10 //Number of runs per given K
	#define dt .001 //Time step
	#define T 5000 //End of simulation time

//For fixed value of K-s in simulation
	#define K1 1.
	#define K2 .1
//For sweeping of K
	#define K0 1.
	#define dK .5
	#define K_max 2. //
	#define PATH_MAX 1000
//---------------------------End Definitions-------------------------//

//Run bools:
	//check values for initial configurations:
	bool check_initial = false;
	//Gaussian distributed frequencies [N(0,1)]
	bool gaussian_frequencies = true;
	//ODE + sweeping K
	bool sweeping_K = false;


//-------------------------Functions Declaration-------------------------//
void CreateResultsFolder();
void PrintParams(float K_run);
float * RandUnifPhase();
float * RandUnifFreq();
float * RandGauss();
float PeriodicPosition(float angular_pos);
void EulerStep(float *phases, float *ang_freqs, float K, double o_par[]);
void OrderParam(float *phases, double o_param[]);
void ClearResultsFile(float K);
float EvaluateMean(float *array, int len_array);
float EvaluateStd(float *array, int len_array, float mean);
void WriteResults(double o_par[], float K, float t_loop);
//----------------------------------------------------------------------//


float iN=(float)(1/N);

//Functions declarartion:

int main(void)    
{	
	clock_t begin_main = clock();
	srand(time(NULL));
	sleep(0);
	CreateResultsFolder();

	int i,j,k;
	float complex iN = (float)(1/N)+I*0; //inverse of N

	float K_range = K_max - K0;
	int number_k_steps = K_range/dK;

	//--------------------START K LOOP--------------------//
	for(j=0;j<number_k_steps;j++)
	{
		printf("RUNNING %d/%d element in K loop\nK=%.4f",j,number_k_steps,j*dK);
		clock_t begin_k_loop = clock();

		float K_run = K0+j*dK;
		//printf("%s", results_path);
		PrintParams(K_run);



		//--------------------START K LOOP--------------------//
			//Declarations

			float *phases;
			float *ang_freqs;

			double ord_param[T+1][4] = {0};		
			double ord_param_acc[n_runs][T+1][2] = {0};		
			//----------------------START MULTIPLE RUNS LOOP----------------------//
			for(k=0;k<n_runs;k++){
				printf("_________________________________________________\n\n");
				printf("\t\tRUN %d/%d, K = %.4f (%d/%d)\n",k+1,n_runs,K_run,j+1,number_k_steps);
				printf("_________________________________________________\n");
				//Initialize phases and frequencies
				phases = RandUnifPhase();
				if(gaussian_frequencies==true){
					ang_freqs= RandGauss();
				}
				else{
					ang_freqs= RandUnifFreq();
				}

					//----------------------START SINGLE RUN LOOP----------------------//
						//printf("InitialPhase=%.5f,\tInitialFrequency%.5f\n",phases[N-1],ang_freqs[N-1]);
							int T_split = (int)(T/3);
							ClearResultsFile(K_run);
							for(i=0;i<T+1;i++){
								OrderParam(phases,ord_param_acc[k][i]);
								EulerStep(phases, ang_freqs, K_run,ord_param_acc[k][i]);
								
								if(i%T_split==0){
									printf("\n\tProcess at %d/100, K=%.4f\n", 100*(int)(i)/T,K_run);
									printf("\tOrderParameter Modulus = %.5f\n",ord_param_acc[k][i][0]);
									printf("\tOrderParameter Phase = %.5f\n",ord_param_acc[k][i][1]);

									}
							}
					//----------------------END SINGLE RUN LOOP----------------------//
				}
				int ii,kk;		
				float stat_modulus[n_runs] = {0}; 
				float stat_phase[n_runs] = {0};
				
				//Perform means, std and save in format:
				//ord_param[0]-[1] => mean/std_modulus		
				//ord_param[2]-[3] => mean/std_phase

				for(ii=0;ii<T+1;ii++){ //for each time step
					for(kk=0;kk<n_runs;kk++){ //accumulate results in one vector
						stat_modulus[kk]=ord_param_acc[kk][ii][0];
						stat_phase[kk]=ord_param_acc[kk][ii][1];
					}
					ord_param[ii][0] = EvaluateMean(stat_modulus,n_runs);
					ord_param[ii][1] = EvaluateStd(stat_modulus,n_runs,ord_param[ii][0]);
					ord_param[ii][2] = EvaluateMean(stat_phase,n_runs);
					ord_param[ii][3] = EvaluateStd(stat_phase,n_runs,ord_param[ii][2]);
					WriteResults(ord_param[ii], K_run, ii);
				}
			//----------------------END MULTIPLE RUNS LOOP----------------------//
			
			PrintParams(K_run);
			clock_t inner_loop_end = clock();
		double loop_time_spent = (double)(inner_loop_end - begin_k_loop) / CLOCKS_PER_SEC;
		printf("\tLoop time on %d runs: %.5f seconds\n\n",n_runs,loop_time_spent);
		//----------------------END SINGLE K LOOP----------------------//
	}
	clock_t outer_loop_end = clock();

	double total_time_spent = (double)(outer_loop_end - begin_main) / CLOCKS_PER_SEC;

	printf("Total execution time: %.5f seconds\n\n",total_time_spent);
  	return 0;
}






//-----------------------------------Function implementations-----------------------------------//

void PrintParams(float K_run){
	printf("\n\n\n//------------------------Input parameters------------------------//\n");
	printf("\t-------> Number of Oscillators = %d\n",N);
	printf("\t-------> Runtime simulation = %d\n",T);
	printf("\t-------> Start K = %.2f\n",K0);
	printf("\t-------> K = %.2f\n",K_run);
	printf("\t-------> dK = %.2f\n",dK);
	printf("\t-------> End K = %.2f\n",K_max);
	printf("\t-------> dt = %.5f\n",dt);
	printf("\t-------> Gaussian initial frequencies? = %s\n", gaussian_frequencies ? "True!" : "False!");
	printf("\t-------> Check initial conditions? = %s\n", check_initial ? "True!" : "False!");
	printf("//----------------------------------------------------------------//\n\n\n");
}

float * RandUnifPhase( ) {
	/* Generate array uniformly distributed of float random variables*/
    static float r[N];
    int i;
    float max_freq = 5; //if max2pi=false, the resulting random uniform distrib will be in [-max_freq,max_freq]
    srand( (unsigned)time(NULL) );
	    for ( i = 0; i < N; ++i){
	      		r[i] = ((float)rand()/(float)(RAND_MAX)) * turn_angle;
	    	}
    return r;
}

float * RandUnifFreq( ){
	/* Generate array uniformly distributed of float random variables*/
    static float r[N];
    int i;
    float max_freq = 5; //if max2pi=false, the resulting random uniform distrib will be in [-max_freq,max_freq]
    srand( (unsigned)time(NULL) );
    for ( i = 0; i < N; ++i) {
		r[i] = ((float)rand()/(float)(RAND_MAX)) * max_freq -max_freq*.5;
    }
    return r;
}

float * RandGauss(  ){
	/*https://www.doc.ic.ac.uk/~wl/papers/07/csur07dt.pdf for gaussian array generation*/
	/*Box-Muller transform*/
	srand( (unsigned)time(NULL) );

    static float r[N];
    float U1,U2,sqrt_term,phase;
    int i;
    for ( i = 0; i < N/2; ++i){
      	U1 = ((float)rand()/(float)(RAND_MAX));
      	U2 = ((float)rand()/(float)(RAND_MAX));
      	
      	sqrt_term = sqrt(-2*log(U1));
      	phase = U2 * turn_angle;
      	r[2*i] = sqrt_term*cos(phase);
      	r[2*i+1] = sqrt_term*sin(phase);
    }
    return r;
}

float PeriodicPosition(float angular_pos){
	if (angular_pos>turn_angle){
		angular_pos-=turn_angle;
	}
	if (angular_pos<0){
		angular_pos+=turn_angle;
	}
	return angular_pos;
}

void OrderParam(float *phases, double o_par[])
{
		int i;
    	double real_ord_param = 0;
    	double imag_ord_param = 0;
    	double iN = 1./((double)N);
		for(i = 0;i < N; i++)
		{
			real_ord_param += iN*cos(phases[i]);
			imag_ord_param += iN*sin(phases[i]);
		}

		o_par[0] = sqrt(real_ord_param*real_ord_param+imag_ord_param*imag_ord_param); //modulus
 		o_par[1] = 2*atan(imag_ord_param/(o_par[0]+real_ord_param)); //Psi
}

//Frequency order parameter!


void EulerStep(float *phases, float *ang_freqs, float K, double o_par[]){
	int i;
	double iN = 1./((double)N);
  	for(i = 0;i < N; i++)
  	{	
  		ang_freqs[i] +=  K*o_par[0]*sin(o_par[1]-phases[i]);
  		phases[i] += dt*ang_freqs[i];
  		//phases[i] = PeriodicPosition(phases[i]); //Not needed: still reads from radians..
  	}
}

void CreateResultsFolder(){
	char path_results[PATH_MAX];
   	if (getcwd(path_results, sizeof(path_results)) != NULL) {
       //printf("Current working dir: %s\n", path_results);
   	} else {
       perror("getcwd() error");
    }
   	strcat(path_results, "/results");
    int ret;
    ret = mkdir(path_results,0777); //creates folder
}

void ClearResultsFile(float K){
		/*Remove File with the same name, avoid overwriting*/
		char filename[64];
		FILE *out;
		if(gaussian_frequencies==true){
			sprintf(filename, "results/gfreq_N%d_T%d_dt%.8f_K%.4f.tsv", N,T,dt,K);
		}
		else{
			sprintf(filename, "results/ufreq_N%d_T%d_dt%.8f_K%.4f.tsv", N,T,dt,K);
		}		if (remove(filename) == 0) 
      		printf("Deleted successfully"); 
   		else
      		printf("Unable to delete the file"); 
}


float EvaluateMean(float *array, int len_array){
		int i;
		float mean = 0;
		float ilen = 1./((double)len_array);

		for(i=0;i<len_array;i++){
			mean += ilen*array[i];
		}
		return mean;
	}

float EvaluateStd(float *array, int len_array, float mean){
		int i;
		float std = 0;
		float ilen = 1./((double)len_array);

		for(i=0;i<len_array;i++){
			std += ilen*pow(array[i]-mean,2);
		}
		return std;
	}

void WriteResults(double o_par[], float K, float t_loop){
		/*Single shot writing of order param and freq order param (both real and complex)*/
		int i;
		char filename[64];
		FILE *out;
		if(gaussian_frequencies==true){
			sprintf(filename, "results/gfreq_N%d_T%d_dt%.8f_K%.4f.tsv", N,T,dt,K);
		}
		else{
			sprintf(filename, "results/ufreq_N%d_T%d_dt%.8f_K%.4f.tsv", N,T,dt,K);
		}
		out = fopen( filename, "a");
		fprintf(out,"%.5f\t%.20f\t%.20f\t%.20f\t%.20f\n",t_loop*dt, o_par[0],o_par[1],o_par[3],o_par[4]);
		fclose(out);
}
