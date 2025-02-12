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
	#define PATH_MAX 1000
//Main parameters
	#define N 2000	 //Number of Kuramoto oscillators
	#define n_runs 10 //Number of runs per given K
	#define dt .01 //Time step
	#define T 20000 //End of simulation time
//For sweeping of K
	#define len_K_list 30
	float K_list[] = { 0.000,0.207,0.414,0.621,0.828,1.034,
						1.241,1.448,1.655,1.862,2.069,2.276,
						2.483,2.690,2.897,3.103,3.310,3.517,
						3.724,3.931,4.138,4.345,4.552,4.759,
						4.966,5.172,5.379,5.586,5.793,6.000 }; 

//For Watts-Strogatz bonus part
	#define r_WS  6. //(already x2)
	struct adj_edges{
		int from [N*(int)(r_WS)];
		int to [N*(int)(r_WS)];
		int dist_deg[N*(int)(r_WS)];
	};
	float p_list[] = {0, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, .95,1};

//---------------------------End Definitions-------------------------//

//Run bools:
	//check values for initial configurations:
	bool check_initial = true;
	//Gaussian distributed natural frequencies [N(0,1)]
	bool gaussian_frequencies = true;
	//Gamma for UnifDistrib of natural freqs:
	#define gamma 10.00
	//MeanField in ODE
	bool WS = true;
	bool mean_field = false;

//-------------------------Functions Declaration-------------------------//
void CreateResultsFolder();
void PrintParams(float K_run, int pvalue_index);
float * ConstVal( float value);
float * RandUnifPhase();
float * RandUnifFreq();
float * RandGauss();
void CopyArray(float *from, float *to);
float PeriodicPosition(float angular_pos);
void EulerStep(float *phases, float *ang_freqs_0, float K, float o_par[], struct adj_edges edges_f);
float* ExtractFreqs(float *vec1, float *vec2);void OrderParam(float *phases, float o_param[]);
void ClearResultsFile(float K, int pvalue_index);
float EvaluateMean(float *array, int len_array);
float EvaluateStd(float *array, int len_array, float mean);
void WriteResults(float o_par[], float K, float t_loop, int pvalue_index);
struct adj_edges read_adj_netw(float p); //for bonus part
//----------------------------------------------------------------------//


float iN=(float)(1/N);

//Functions declarartion:

int main(void)    
{	
	// float *from_test;
	// from_test = RandGauss();
	// float to_test[N]={0};
	// CopyArray(from_test,to_test);
	// exit(0);
	clock_t begin_main = clock();
	srand(time(NULL));
	sleep(0);
	CreateResultsFolder();

	int i,j,k,p;  //i loops over time, j loops over K values, k loops over runs, p loops over pvalues for WS
	float complex iN = (float)(1/N)+I*0; //inverse of N

	//Read adjacency matrix and save is into the structure 
	struct adj_edges edges[sizeof(p_list)/sizeof(p_list[0])]; //for each of the p simulated
 	for(k=0; k<sizeof(p_list)/sizeof(p_list[0]-1);k++){	//k loop, for each of the p value analyzed, up to cardinality of p_list
 		edges[k]=read_adj_netw(p_list[k]);
	}
	//--------------------START p LOOP--------------------//
	for(p=0; p<sizeof(p_list)/sizeof(p_list[0]-1);p++){	
		//--------------------START K LOOP--------------------//
		for(j=0;j<len_K_list;j++){
			clock_t begin_k_loop = clock();

			float K_run = K_list[j];
			PrintParams(K_run,p);

			//Declarations
			float *phases;
			float *ang_freqs;
			float *dummy_phases; //Just for extracting ang_freqs
			float *ang_freqs_0; //Natural frequencies of the oscillators
			//ADD ACCUMULATORS FOR PHASES AND ANG FREQS SO THAT AVG AND STD CAN BE DERIVED, check eu_test.c
			
			float ord_param[T+1][4] = {0};		
			float ord_param_acc[n_runs][T+1][2] = {0};		
			
			//Too large to keep one whole matrix: avg and std will be evaluated at each time step and then averaged on different runs
			//double phase_acc[n_runs][(T+1)/500][N] = {0};
			//----------------------START MULTIPLE RUNS LOOP----------------------//
				for(k=0;k<n_runs;k++){
					clock_t check_elapsed_time = clock();
					double check_elapsed = (double)(check_elapsed_time - begin_main) / CLOCKS_PER_SEC;
				
					printf("_________________________________________________________________\n\n");
					printf("\t\tK = %.4f (%d/%d), RUN %d/%d, \n",K_run,j+1,len_K_list,k+1,n_runs);
					printf("\t\tTotal elapsed time =  %.5f seconds\n",check_elapsed);
					printf("\t\tTotal Progress = %d/%d (%.6f/100) \n",j*n_runs+k+1,(len_K_list+1)*n_runs,((float)(j*n_runs+k))/((float)(len_K_list*n_runs)));
					printf("_________________________________________________________________\n");
					
			/////////Initialize phases and frequencies/////////
					if(gaussian_frequencies==true){
						ang_freqs = RandGauss();
						ang_freqs_0 = ang_freqs;
					}
					else{
						ang_freqs = RandUnifFreq();
						ang_freqs_0 = ang_freqs;
					}
					
					phases = RandUnifPhase();				
					
					if(check_initial==true){
						float mean_phases = 0;
						float std_phases = 0;		

						float mean_freqs = 0;
						float std_freqs = 0;

						mean_phases = EvaluateMean(phases,N);
						mean_freqs = EvaluateMean(ang_freqs,N);

						printf("\nMean Phases0  = %.5f\n",mean_phases);
						printf("\nMean Freqs0 = %.5f\n",mean_freqs);
						std_phases = EvaluateStd(phases,N,mean_phases);
						std_freqs = EvaluateStd(ang_freqs,N,mean_freqs);
						printf("\nStd Phases0 = %.5f\n",std_phases);
						printf("\nStd Freqs0 = %.5f\n\n",std_freqs);
					}
					//exit(0);
						//----------------------START SINGLE RUN LOOP----------------------//
								int T_split = (int)(T/5);

								ClearResultsFile(K_run, p);
								clock_t begin_single_loop = clock();
								for(i=0;i<T+1;i++){
									OrderParam(phases,ord_param_acc[k][i]);
									EulerStep(phases, ang_freqs_0, K_run,ord_param_acc[k][i],edges[p]);
									//CopyArray(phases,phases);
									// if(i%T_split==0){
									// 	printf("\n\tProcess at %d/100, K=%.4f\n", 100*(int)(i)/T,K_run);
									// 	printf("\tOrderParameter Modulus = %.5f\n",ord_param_acc[k][i][0]);
									// 	printf("\tOrderParameter Phase = %.5f\n",ord_param_acc[k][i][1]);
									// 	}
								}
							clock_t stop_single_loop = clock();
							double T_loop_time_spent = (double)(stop_single_loop - begin_single_loop) / CLOCKS_PER_SEC;
							printf("\n\tLoop time on single run =  %.5f seconds\n\n",T_loop_time_spent);

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
							
							//	Choose not to use 2π periodicity here so that std is not affected, 
							//	in post processing the Psi will undergo mod(2π) before filling the plots
							//	while std won't
						
						ord_param[ii][2] = EvaluateMean(stat_phase,n_runs); 
						ord_param[ii][3] = EvaluateStd(stat_phase,n_runs,ord_param[ii][2]);
						WriteResults(ord_param[ii], K_run, ii,p);
					}
			//----------------------END MULTIPLE RUNS LOOP----------------------//

			clock_t inner_loop_end = clock();
			double K_loop_time_spent = (double)(inner_loop_end - begin_k_loop) / CLOCKS_PER_SEC;
			printf("\tLoop time on %d runs: %.5f seconds\n\n",n_runs,K_loop_time_spent);
			//----------------------END SINGLE K LOOP----------------------//
	}
	//--------------------END p LOOP--------------------//

	}
	clock_t outer_loop_end = clock();

	double total_time_spent = (double)(outer_loop_end - begin_main) / CLOCKS_PER_SEC;

	printf("Total execution time: %.5f seconds\n\n",total_time_spent);
  	return 0;
}

//-----------------------------------Function implementations-----------------------------------//

void CopyArray(float *from, float *to){
	int i;
	for(int i = 0; i < N; ++i){
		printf("Step %d:",i);
		to[i] = from[i];
		printf("Value %.3f\n",to[i]);

	}
}

void PrintParams(float K_run,int pvalue_index){
	printf("\n\n\n//------------------------Input parameters------------------------//\n");
	printf("\n\t Number of Oscillators = %d\n",N);
	printf("\t Runtime simulation = %d\n",T);
	printf("\t dt = %.5f\n",dt);
	printf("\t n_runs_per_K = %d\n",n_runs);
	printf("\t K = %.4f\n",K_run);
	printf("\t p = %.3f\n",p_list[pvalue_index]);
	printf("\t MeanField EulerStep? = %s\n", mean_field ? "True!" : "False!");
	printf("\t Gaussian initial frequencies? = %s\n", gaussian_frequencies ? "True!" : "False!");
	if(gaussian_frequencies==false){
		printf("\t Gamma = %.1f",gamma);
	}
	printf("\t Check initial conditions? = %s\n\n", check_initial ? "True!" : "False!");
	printf("//----------------------------------------------------------------//\n\n\n");
}

float * RandUnifPhase() { 
	/* Generate array uniformly distributed of float random variables*/
    static float r[N];
    int i;
    float max_phase = (float) 2*M_PI; 
	    for ( i = 0; i < N; ++i){
	      		r[i] = ((float)rand()/(float)(RAND_MAX)) * max_phase - max_phase*.5; //here something wrong with radians/angle conversion is happening
	    	}
    return r;
}


float * ConstVal( float value ){
	//Useless, just a simple vector initialization to given value was enough
    static float r[N];
    int i;
    for (i = 0; i < N; ++i) {
		r[i] = value;
    }
    return r;
}

float * RandUnifFreq(){ 
	/* Generate array uniformly distributed of float random variables*/
    static float r[N];
    int i;
    float max_freq = .5; 
    for ( i = 0; i < N; ++i) {
		r[i] = ((float)rand()/(float)(RAND_MAX)) * max_freq - max_freq*.5;
    }
    return r;
}

float * RandGauss(){ 
	/*https://www.doc.ic.ac.uk/~wl/papers/07/csur07dt.pdf for gaussian array generation*/
	/*Box-Muller transform*/
    static float r[N];
    float U1,U2,sqrt_term,phase;
    int i;
    for ( i = 0; i < N/2; ++i){  //N/2 as this method returns couples of random gaussian distributed numbers
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

void OrderParam(float *phases, float o_par[]){
		int i;
    	double real_ord_param = 0;
    	double imag_ord_param = 0;
    	double iN = 1./((double)N);
		for(i = 0;i < N; i++){
			real_ord_param += iN*cos(phases[i]);
			imag_ord_param += iN*sin(phases[i]);
		}
		o_par[0] = sqrt(real_ord_param*real_ord_param+imag_ord_param*imag_ord_param); //modulus
 		o_par[1] = 2*atan(imag_ord_param/(o_par[0]+real_ord_param)); //Psi
}

void EulerStep(float *phases, float *ang_freqs_0, float K, float o_par[], struct adj_edges edges_f){
	//o_par = Order Parameters, ang_freqs_0 = Natural frequencies
	// o_par[0] == modulus, o_par[1] == Psi 
	int i,j;
	if(WS==false){
		double iN = 1./((double)N);
	  	if(mean_field==false){ //Mutual interaction
		  	for(i = 0;i < N; i++){	
		  		phases[i] = phases[i] + dt * ang_freqs_0[i];
		  		for(int j = 0; j < N; j++){
		        	phases[i] += dt * K * iN * sin(phases[j]-phases[i]);
		    	}
		  	}
	  	}
	  	else{ //Mean field
	  		float interaction_term;
		  	for(i = 0;i < N; i++){	
		  		//Evaluate interactions
		  		interaction_term = K*o_par[0]*sin(o_par[1]-phases[i]);
		  		//Update phases
		  		phases[i] += (ang_freqs_0[i] + interaction_term)*dt;
		  	}
	  	}
  	}
  	else{
		for(i = 0;i < N; i++){	//Update phase with natural frequencies
			phases[i] = phases[i] + dt * ang_freqs_0[i]; 
		}
		for(i = 0; i< N *(int)(r_WS);i++){ // Update with interactions
		      	phases[edges_f.from[i]] += dt * K * 1./edges_f.dist_deg[i] * sin(phases[edges_f.to[i]]-phases[edges_f.from[i]]);
		   }
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

void ClearResultsFile(float K, int pvalue_index){

		/*Remove File with the same name, avoid overwriting*/
		char filename[100];
		char MF[10];
		char phase0_name[10];
		FILE *out;
		if(mean_field==true){
			sprintf(MF,"MF");
		}
		else{
			sprintf(MF,"NOMF");
		}
		sprintf(phase0_name,"uphase");

		if(gaussian_frequencies==true){
			sprintf(filename, "results/WS_gfreq_%s_N%d_%s_T%d_dt%.4f_nruns%d_K%.3f_p=%.3f.tsv", phase0_name, N,MF,T,dt,n_runs,K, p_list[pvalue_index]);
		}
		else{
			if(gamma!=.5){
				sprintf(filename, "results/WS_ufreq_gamma%.2f_%s_N%d_%s_T%d_dt%.4f_nruns%d_K%.3f_p=%.3f.tsv", gamma, phase0_name, N,MF,T,dt,n_runs,K, p_list[pvalue_index]);
			}
			else{
				sprintf(filename, "results/WS_ufreq_%s_N%d_%s_T%d_dt%.4f_nruns%d_K%.3f_p=%.3f.tsv", phase0_name, N,MF,T,dt,n_runs,K, p_list[pvalue_index]);

			}
		}

		if (remove(filename) == 0) 
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

void WriteResults(float o_par[], float K, float t_loop, int pvalue_index){
		/*Single shot writing of order param and freq order param (both real and complex)*/
		int i;
		char filename[100];
		char MF[10];
		char phase0_name[10];

		FILE *out;
		if(mean_field==true){
			sprintf(MF,"MF");
		}
		else{
			sprintf(MF,"NOMF");
		}
		sprintf(phase0_name,"uphase");
		
		if(gaussian_frequencies==true){
			sprintf(filename, "results/WS_gfreq_%s_N%d_%s_T%d_dt%.4f_nruns%d_K%.3f_p=%.3f.tsv", phase0_name, N,MF,T,dt,n_runs,K, p_list[pvalue_index]);
		}
		else{
			if(gamma!=.5){
				sprintf(filename, "results/WS_ufreq_gamma%.2f_%s_N%d_%s_T%d_dt%.4f_nruns%d_K%.3f_p=%.3f.tsv", gamma, phase0_name, N,MF,T,dt,n_runs,K, p_list[pvalue_index]);
			}
			else{
				sprintf(filename, "results/WS_ufreq_%s_N%d_%s_T%d_dt%.4f_nruns%d_K%.3f_p=%.3f.tsv", phase0_name, N,MF,T,dt,n_runs,K, p_list[pvalue_index]);

			}
		}
		out = fopen( filename, "a");
		fprintf(out,"%.5f\t%.20f\t%.20f\t%.20f\t%.20f\n",t_loop*dt, o_par[0],o_par[1],o_par[2],o_par[3]);
		fclose(out);
}

//For bonus part
struct adj_edges read_adj_netw(float p){
    //Get path for results of Python simulation of Watts-Strogatz
    static char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
       //printf("Current working dir: %s\n", cwd);
    } else {
       perror("getcwd() error");
    }
    //Get filename for the edges list

  	if(p==0 || p==1){
  		if (p==0){
  		strcat(cwd, "/results_WS/extended_adj_mat_p=");
	    strcat(cwd,"0.00"); 
	    strcat(cwd,".csv"); 
		}
		else{
		strcat(cwd, "/results_WS/extended_adj_mat_p=");
		strcat(cwd,"1.00"); 
		strcat(cwd,".csv"); 
		}
  	}
  	else{
  		int decimal, sign;  //needed
		strcat(cwd, "/results_WS/extended_adj_mat_p=0.");
		char *buffer = ecvt(p, 2, &decimal, &sign);
	    strcat(cwd,buffer); 
	    strcat(cwd,".csv"); 
	}
   	printf("Filename: %s\n", cwd);

    //Read data and store in struc
	int i;
    struct adj_edges edges;
    FILE *fp; 
	if ( ( fp = fopen(cwd, "rb" ) ) == NULL ){
         printf( "\n\nALERT: File %s could not be opened\n\n",cwd);

    }
    
    printf("\nNow reading from file..\n");
	for(i=0; i<N*r_WS;i++)
	    {
	    	fscanf(fp, "%d,%d,%d\n",&edges.from[i],&edges.to[i],&edges.dist_deg[i]);
	    }
	fclose(fp);
    return edges;
}

float* ExtractFreqs(float *vec1, float *vec2){
	//Used together with dummy_phases to extract frequencies at each time step
	int i;
    static float result[N];
	for (int i = 0; i < N; ++i)
	{
		result[i] = (vec1[i]-vec2[i])/dt;
	}
	return result;
}
