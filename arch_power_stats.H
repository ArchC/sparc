#ifdef POWER_SIM
#include <powersc.h>

/* Data struct definition. You should think that it is a row in a table. Each profile will have a certain number of tables. 
	 The basic idea is use a profile, with a pre-fixed number of operational frequencies. Each frequency, with a specific 
	 table of values */

// This group should be parameters, not defines
#define START_PROFILE 0
#define NUM_INSTR 119
//#define POWER_TABLE_FILE "../../processors/sparc/acpower_table.csv"
#define POWER_TABLE_FILE "../../processors/sparc/acpower_table_sparc_xc3s1000_40Mhz.csv"
//#define POWER_TABLE_FILE "../../processors/sparc/acpower_table_sparc_xc6slx75_40Mhz.csv"



#define WINDOW_REPORT

//#define WINDOW_REPORT_FILE "window_power_report.csv"
#define WINDOW_REPORT_FILE "window_power_report"



#define START_WINDOW_SIZE 1000000

#define MAX_LINESIZE_CSV_FILE 10240 // Inefficient and non-scalable
#define MAX_INSTR_NAME_SIZE 30
#define MAX_POWER_STATS_NAME_SIZE 30
#define MAX_POWER_STATS_DESCR_SIZE 140

#define TYPE_LINE_NUM_PROFILE 0
#define TYPE_LINE_PROFILE     1
#define TYPE_LINE_STALL       2
#define TYPE_LINE_OP          3

class power_stats {
	private:
		struct profile {
			char power_stats_name[MAX_POWER_STATS_NAME_SIZE];
			char power_stats_descr[MAX_POWER_STATS_DESCR_SIZE];
			unsigned int freq;
			double freq_scale;
			double power_scale;
			double power[NUM_INSTR+1];
      double stall_power;
		};

		struct power_stats_data {
			char instr_name[NUM_INSTR+1][MAX_INSTR_NAME_SIZE];
			profile* p;
		};

		struct dynamic_data {
#ifdef WINDOW_REPORT
			long long window_num_instr;
			double window_energy;
			double window_power;
			long long window_count;
			unsigned int window_size;
		  double execution_time;
#endif

			long long total_num_instr; 
			double total_energy;
			double total_power;

			unsigned int actual_profile;
			unsigned int num_profiles;
		};

		dynamic_data dyn;
		power_stats_data psc_data;

#ifdef WINDOW_REPORT
		FILE* out_window_power_report;
#endif

	public:
		psc_cell_power_info psc_info;

		// Constructor
		power_stats(const char* proc_name): psc_info(proc_name, "Processor") 

	{

	
      PSC_NUM_FIRST_SAMPLES(0x7FFFFFFF);
			init(POWER_TABLE_FILE);
			dyn.actual_profile = START_PROFILE;
			
			dyn.total_num_instr = 0;
			dyn.total_energy = 0;
			dyn.total_power = 0;
#ifdef WINDOW_REPORT
			dyn.window_size = START_WINDOW_SIZE;

			dyn.window_num_instr = 0;
			dyn.window_energy = 0;
			dyn.window_power = 0;
			dyn.window_count = 0;
			dyn.execution_time = 0;

			/****/
			char filename[512];
			strcpy(filename, WINDOW_REPORT_FILE);
			strcat(filename, "_");
			strcat(filename, proc_name);
			strcat(filename, ".csv");
			out_window_power_report = fopen(filename, "w");
			if (out_window_power_report == NULL) {
				perror("Couldn't open specified out_window_power_report file");
				exit(1);
			}
			/****/
#endif
			//print_psc_data();
		}

		// Destructor
		~power_stats() {
			free(psc_data.p);

#ifdef WINDOW_REPORT
			fclose(out_window_power_report);
#endif
		}

		double get_power() {
			return psc_info.get_power();
		}

		double get_power_instruction(int id, int profile) {
      // [J] * [1/s] = [W]
			return (psc_data.p[profile].power[id] * psc_data.p[profile].power_scale * 
        psc_data.p[profile].freq_scale * psc_data.p[profile].freq);
		}

    int type_line(int line, int num_profiles) {
      if (num_profiles == 0) {
        return TYPE_LINE_NUM_PROFILE;
      } 
      else if (line <= num_profiles + 1) {
        return TYPE_LINE_PROFILE;
      }
      else if (line == num_profiles + 2) {
       return TYPE_LINE_STALL;
      }
      else
        return TYPE_LINE_OP;
    }

#ifdef WINDOW_REPORT
		void incr_window_energy(double v) {
			dyn.window_energy += v;
		}

		// double estimate_execution_time() {
		//	dyn.execution_time += dyn.window_count / (psc_data.p[dyn.actual_profile].freq * psc_data.p[dyn.actual_profile].freq_scale);
		//	return dyn.execution_time;
		// }

		void reset_window_data() {
			dyn.window_num_instr = 0;
			dyn.window_energy = 0;
			dyn.window_power = 0;
		}

		void calc_window_power() {
			dyn.window_power = dyn.window_energy / dyn.window_num_instr;
		}

		void window_power_report() {
			//estimate_execution_time();
			fprintf(out_window_power_report, "%d,%.10lf,%lld,%.10lf\n", dyn.actual_profile, dyn.execution_time, dyn.window_count, dyn.window_power);
		}
#endif

		void incr_total_energy(double v) {
			dyn.total_energy += v;
		}

    void incr_execution_time(int num_instr, int p) {
		  dyn.execution_time += num_instr / (psc_data.p[dyn.actual_profile].freq * psc_data.p[dyn.actual_profile].freq_scale);
    }

		void update_stat_power(int instr_id) {
  		dyn.total_num_instr++;
			incr_total_energy(get_power_instruction(instr_id, dyn.actual_profile));
      incr_execution_time(1, dyn.actual_profile);
      //if (dyn.total_num_instr % 10000 == 0) {
      //  printf("[%10lld] %4d %10.5lf %10.5lf\n", dyn.total_num_instr, instr_id, get_power_instruction(instr_id, dyn.actual_profile), dyn.total_energy); 
      //}
#ifdef WINDOW_REPORT
			dyn.window_num_instr++;
			incr_window_energy(get_power_instruction(instr_id, dyn.actual_profile));
			if (dyn.window_num_instr == dyn.window_size) {
				dyn.window_count++;
				calc_window_power();
				window_power_report();
				reset_window_data();
				//if (dyn.window_count == 360) dyn.actual_profile = 1;
			}
#endif
		}

		void calc_total_power() {
			dyn.total_power = dyn.total_energy / dyn.total_num_instr;
		}

		void powersc_connect() {
			calc_total_power();
			psc_info.set_power(dyn.total_power);
			psc_info.set_level(PSC_RT_LEVEL);
			PSC_INSERT_CELL(psc_info);
		}

		void report() {
			PSC_REPORT_POWER;
		}

		char* next_strtok(const char* param, FILE* f, int pos_line) {
			char* pch = NULL;
			pch = strtok(NULL,param);
			if (pch == NULL) {
				printf("Error reading csv file, line %d. Unexpected format\n", pos_line);
				fclose(f);
				exit(1);
			}
			return pch;
		}

		// Read from file 
		void init(const char* filename) {
			FILE* f = NULL;
			char c = 0;
			char line[MAX_LINESIZE_CSV_FILE];
			char instr_name[MAX_INSTR_NAME_SIZE];
			char aux[MAX_LINESIZE_CSV_FILE];
			double value;
      char* res;

			unsigned int index = 0;
			unsigned int pos_line = 0;
			unsigned int valid_line = 0;
      unsigned int profile_id = 0;
			fpos_t pos;

			char* pch = NULL;

			f = fopen(filename, "r");
			if (f == NULL) {
				sprintf(aux, "Power file %s not found", filename);
				perror(aux);
				exit(1);
			}
      
      dyn.num_profiles = 0; // Set a default value 

			do {
				//fgetpos (f, &pos);
				res = fgets(line, MAX_LINESIZE_CSV_FILE, f); // Get a new line from file
				if (feof(f)) break;
				pos_line++; // It says what line I am reading now
				pch = strtok(line, ",\""); // It parses the line

				if (pch[0] == '#'); // If it is a comment, ignore
				else if (pch == NULL) break;
				else { // Just found a valid new line
					valid_line++;
					// First Valid Line: number of profiles
          switch(type_line(valid_line, dyn.num_profiles)) {
            case TYPE_LINE_NUM_PROFILE:
              dyn.num_profiles = atoi(pch);
					  	psc_data.p = (profile *)malloc(sizeof(profile) * dyn.num_profiles);
              // Cleaning table
              for(int j = 0; j <= NUM_INSTR; j++) {
                strcpy(psc_data.instr_name[j], "");
                for (int i = 0; i < dyn.num_profiles; i++) {
                  psc_data.p[i].power[j] = 0;
                }
              }
            break;
            case TYPE_LINE_PROFILE:
					  	profile_id = atoi(pch);

              if (profile_id >= dyn.num_profiles) {
                printf("Error: Invalid profile_id greater than num_profiles: %d > %d\n", 
                  profile_id, dyn.num_profiles);
              }
    					
              pch = next_strtok(",\"", f, pos_line);
    					psc_data.p[profile_id].freq = atoi(pch);
    					
    					pch = next_strtok(",\"", f, pos_line);
    					psc_data.p[profile_id].freq_scale = atof(pch);
    					
    					pch = next_strtok(",\"", f, pos_line);
    					psc_data.p[profile_id].power_scale = atof(pch);
    					
    					pch = next_strtok(",\"", f, pos_line);
    					strcpy(psc_data.p[profile_id].power_stats_name, pch);
    					
    					pch = next_strtok(",\"", f, pos_line);
    					strcpy(psc_data.p[profile_id].power_stats_descr, pch);
            break;
            case TYPE_LINE_STALL:
    					psc_data.p[0].stall_power = atof(pch);

    					for(int i = 1; i < dyn.num_profiles;i++) {
    						pch = next_strtok(",\"", f, pos_line);
    						psc_data.p[i].stall_power = atof(pch);
    					}
            break;
            default: // TYPE_LINE_OP
    					index = atoi(pch);

              if (index <= NUM_INSTR) {
    					  pch = next_strtok(",\"", f, pos_line);
    					  strcpy(psc_data.instr_name[index], pch);
    					  for(int i = 0; i < dyn.num_profiles;i++) {
    					  	pch = next_strtok(",\"", f, pos_line);
    					  	psc_data.p[i].power[index] = atof(pch);
    					  }
              }
            break;
          }
				}
			} while(!feof(f));

			fclose(f);
		}

		void print_psc_data() {
			int i = 0, p = 0;

			for(p = 0; p < dyn.num_profiles; p++) {
				printf("Profile %d\n", p);
				printf("Name: %s\n", psc_data.p[p].power_stats_name);
				printf("Description: %s\n\n", psc_data.p[p].power_stats_descr);
			}

			printf("Instr ID | Instruction Name");
			for(p = 0; p < dyn.num_profiles; p++) {
				printf(" | Power Profile %d", p);
			}
			printf("\n");
			for(i = 1; i <= NUM_INSTR; i++) {
				printf("%8d | %16s", i, psc_data.instr_name[i]);
				for(p = 0; p < dyn.num_profiles; p++) {
					printf(" | %15.3lf", psc_data.p[p].power[i]);
				}
				printf("\n");
			}
		}
};
#endif
