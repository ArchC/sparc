#ifdef POWER_SIM
#include <powersc.h>
#include <systemc>

/* Data struct definition. You should think that it is a row in a table. Each profile will have a certain number of tables. 
	 The basic idea is use a profile, with a pre-fixed number of operational frequencies. Each frequency, with a specific 
	 table of values */

// This group should be parameters, not defines

#define NUM_INSTR 119
//#define POWER_TABLE_FILE "acpower_table_sparc_spartan_50Mhz.csv"
//#define POWER_TABLE_FILE "acpower_table_sparc_xc3s1000_40Mhz.csv"
//#define POWER_TABLE_FILE "acpower_table_sparc_xc3s1200e_40Mhz.csv"
//#define POWER_TABLE_FILE "acpower_table_sparc_xc5vlx50t_40Mhz.csv"
#define POWER_TABLE_FILE "acpower_table_sparc_xc6slx75_40Mhz.csv"

#define WINDOW_REPORT
#define WINDOW_REPORT_FILE "window_power_report"
#define START_WINDOW_SIZE 1000

#define MAX_LINESIZE_CSV_FILE 10240 // Inefficient and non-scalable
#define MAX_INSTR_NAME_SIZE 30
#define MAX_POWER_STATS_NAME_SIZE 30
#define MAX_POWER_STATS_DESCR_SIZE 140

#define TYPE_LINE_NUM_PROFILE 0
#define TYPE_LINE_PROFILE     1
#define TYPE_LINE_STALL       2
#define TYPE_LINE_OP          3


#define CYCLES_PER_FREQUENCY_EXCHANGE 20000 // nanoseconds = or 20 micro seconds
#define CYCLES_TO_RESTART 300

//#define DEBUG

class power_stats {
	private:
		struct profile
		{
			char power_stats_name[MAX_POWER_STATS_NAME_SIZE];
			char power_stats_descr[MAX_POWER_STATS_DESCR_SIZE];

			unsigned int freq;
			double freq_scale;
			double power_scale;
			double power[NUM_INSTR+1];
      		double stall_power;
    		
		};

		struct power_stats_data
		{
			char instr_name[NUM_INSTR+1][MAX_INSTR_NAME_SIZE];
			profile* p;
			int index_nop;
		};

		struct dynamic_data
		{
			#ifdef WINDOW_REPORT
			long long window_num_instr;
			double window_energy;
			double window_power;
			long long window_count;
			unsigned int window_size;
		  	#endif


			double execution_time;
			sc_core::sc_time system_time;
			long long total_num_instr; 
			double total_energy;
			double total_power;

			/*****/
			double edp;
			double energy_per_core;

			int delta_instr;
			int last_delta_instr;

			double delta_t;

			unsigned int actual_profile;
			unsigned int num_profiles;

			bool freq_changed;
		};

		dynamic_data dyn;
		power_stats_data psc_data;
		
		
		#ifdef WINDOW_REPORT
		FILE* out_window_power_report;
		
		#endif

		#ifdef DEBUG 
		FILE* debug_file;
		int contador_debug;
		#endif




	public:
		psc_cell_power_info psc_info;

		// Constructor
		power_stats(const char* proc_name): psc_info(proc_name, "Processor")
		{
    		PSC_NUM_FIRST_SAMPLES(0x7FFFFFFF);
			init(POWER_TABLE_FILE);
			
			/*Initialize power state using profile 0*/
			dyn.actual_profile = 0;
			dyn.total_num_instr = 0;
			dyn.total_energy = 0;
			dyn.total_power = 0;

			/******/
			dyn.edp = 0;
			dyn.energy_per_core = 0;
			dyn.delta_instr = 0;
			dyn.delta_t = 0;
			dyn.last_delta_instr = 0;
				
			dyn.freq_changed = false;

			
			char filename[512];

			#ifdef WINDOW_REPORT
			dyn.window_size = START_WINDOW_SIZE;
			dyn.window_num_instr = 0;
			dyn.window_energy = 0;
			dyn.window_power = 0;
			dyn.window_count = 0;
			dyn.execution_time = 0;
			dyn.system_time = sc_time(0,SC_NS);

			/****/
			
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
			
			#ifdef DEBUG 
			strcpy (filename, "debug_power");
			strcat(filename, "_");
			strcat(filename, proc_name);
			strcat(filename, ".txt");
			
			debug_file = fopen(filename, "w");
			if (debug_file == NULL) {
				perror("Couldn't open specified debug file");
				exit(1);
			}
			contador_debug = 0;
						
			print_psc_data();
			#endif
		}
	

		// Destructor
		~power_stats()
		{
			free(psc_data.p);

			#ifdef WINDOW_REPORT
			fclose(out_window_power_report);
			#endif

			#ifdef DEBUG
			fclose(debug_file);
			#endif
		}

		double get_power() {
			return psc_info.get_power();
		}

		double get_power_instruction(int id, int profile)
		{
      		// [J] * [1/s] = [W]

			double power = psc_data.p[profile].power[id] * psc_data.p[profile].power_scale * psc_data.p[profile].freq_scale * psc_data.p[profile].freq;

			#ifdef DEBUG
			fprintf(debug_file,"\nGetting power instruction.");
			fprintf(debug_file,"\nprofile: %d\t id: %d\t power[id]*power_scale*freq_scale*freq = %f * %f * %f * %d" , profile, id, psc_data.p[profile].power[id], psc_data.p[profile].power_scale, psc_data.p[profile].freq_scale, psc_data.p[profile].freq);
			fprintf(debug_file,"\nReturning: %f", power);
			contador_debug++;
			#endif
			
			return power;
		}

		double update_energy (int id, int profile)
		{

			//printf("\nupdate_energy id=%d  profile=%d", id, profile);

			double energy_per_instruction = psc_data.p[profile].power[id]; // * psc_data.p[profile].power_scale;
			
			set_edp(dyn.edp + energy_per_instruction);
			dyn.energy_per_core = dyn.energy_per_core + energy_per_instruction;
		}

	
		double get_energy_stamp (int prof)
		{

			dyn.delta_instr = get_total_num_instr() - dyn.delta_instr;  // total de instr executadas no delta_T atual
			int freq = psc_data.p[prof].freq; // freq em MegaHz
			double cycle_time_ns = 1000/freq;  // tempo de um ciclo em nanossegundos
			double edp = get_edp();

			double energyStampUsingFreq = (double) edp*dyn.delta_instr*cycle_time_ns;
		
			dyn.delta_t = sc_time_stamp().to_seconds() - dyn.delta_t;


			double t = dyn.delta_t*1000000000;    // em nanossegundos

			
			double energyStampUsingTime = edp*t;

			return energyStampUsingTime;
		}	

		void initialize_energy_stamp()
		{
			dyn.edp = 0.0;
			dyn.delta_instr = 0;

		}

		double get_edp ()
		{
			return dyn.edp;
		}
		void set_edp (double value)
		{
			dyn.edp = value;
		}
    	int type_line(int line, int num_profiles)
    	{
      		if (num_profiles == 0)
      		{
        		return TYPE_LINE_NUM_PROFILE;
      		} 
      		else if (line <= num_profiles + 1)
      		{
        		return TYPE_LINE_PROFILE;
      		}
      		else if (line == num_profiles + 2)
      		{
        		return TYPE_LINE_STALL;
      		}
      		else
        		return TYPE_LINE_OP;
    	}

		#ifdef WINDOW_REPORT
		void incr_window_energy(double v)
		{
			dyn.window_energy += v;
		}

		void reset_window_data()
		{
			dyn.window_num_instr = 0;
			dyn.window_energy = 0;
			dyn.window_power = 0;
		}

		void calc_window_power()
		{

			dyn.window_power = dyn.window_energy / dyn.window_num_instr;
		}

		void window_power_report()
		{
			
			fprintf(out_window_power_report, "%d,%.10lf,%lld,%.10lf\n", dyn.actual_profile, dyn.execution_time, dyn.window_count, dyn.window_power);
		}
		#endif

		void incr_total_energy(double v) {
			dyn.total_energy += v;

		}

    	void incr_execution_time(int num_instr, int p)
    	{

    		dyn.system_time = sc_time_stamp ();

			dyn.execution_time += num_instr / (psc_data.p[dyn.actual_profile].freq * psc_data.p[dyn.actual_profile].freq_scale);
		
    	}

		void update_stat_power(int instr_id, int n = 1)
		{

			#ifdef DEBUG 
			if (n!=1)
			{	
				printf("\nPOWER_STATS: calculating power after change frequency.");
				printf("\nPOWER_STATS: NOP id: %d  , NOP power: %f",instr_id, get_power_instruction(instr_id, dyn.actual_profile));
			}	
			#endif

  			dyn.total_num_instr = dyn.total_num_instr + n;
			incr_execution_time(n, dyn.actual_profile);

			incr_total_energy(n * get_power_instruction(instr_id, dyn.actual_profile));
     		

     		update_energy(instr_id, dyn.actual_profile);

			#ifdef WINDOW_REPORT

			dyn.window_num_instr = dyn.window_num_instr + n;

			
			incr_window_energy(n* get_power_instruction(instr_id, dyn.actual_profile));

		
			if (dyn.window_num_instr >= dyn.window_size)
			{
				dyn.window_count++;
				calc_window_power();
				window_power_report();
				reset_window_data();
			}
			#endif

		}


		double get_total_num_instr ()
		{
			return dyn.total_num_instr;
		}
		double get_total_energy()
		{
			return dyn.total_energy;
		}

		double get_total_power ()
		{
			calc_total_power();
			return dyn.total_power;
		}
		void calc_total_power()
		{
			dyn.total_power = dyn.total_energy / dyn.total_num_instr;

			#ifdef DEBUG
			fprintf(debug_file,"\n\nCalculating total power = %f:", dyn.total_power);
			#endif
		}

		void powersc_connect()
		{
			calc_total_power();

			// teste
			psc_info.set_power(dyn.total_power);
			psc_info.set_level(PSC_RT_LEVEL);
			PSC_INSERT_CELL(psc_info);

		}

		void report()
		{
			PSC_REPORT_POWER;
			dyn.system_time = sc_time_stamp();
			
		}

		double getEnergyPerCore()
		{
			return dyn.energy_per_core;
		}
		char* next_strtok(const char* param, FILE* f, int pos_line)
		{
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
		void init(const char* filename)
		{
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

             // Get self PATH
            char buff[1024];
            strcpy (buff, POWER_SIM);
            strcat (buff, "/");
            strcat (buff, filename);

			f = fopen(buff, "r");
			if (f == NULL) {
				sprintf(aux, "Power file %s not found", buff);
				perror(aux);
				exit(1);
			}
      
      		dyn.num_profiles = 0; // Set a default value 

      		int state_id = 0;

			do {
				//fgetpos (f, &pos);
				res = fgets(line, MAX_LINESIZE_CSV_FILE, f); // Get a new line from file
				if (feof(f)) break;
				pos_line++; // It says what line I am reading now
				pch = strtok(line, ",\""); // It parses the line


				if (pch[0] == '#'); // If it is a comment, ignore
				else if (pch == NULL) break;
				else
				{ // Just found a valid new line
					valid_line++;
					// First Valid Line: number of profiles
          			switch(type_line(valid_line, dyn.num_profiles))
          			{
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
					  		
            				profile_id = state_id++;

            				if (profile_id >= dyn.num_profiles) {
                				printf("Error: Invalid profile_id greater than num_profiles: %d > %d\n", 
                  						profile_id, dyn.num_profiles);
              				}
    					
    						//pch = next_strtok(",\"", f, pos_line);
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
    					  		if (!strcmp(pch,"nop")) psc_data.index_nop = index;  // capture the  NOP index

    					  		for(int i = 0; i < dyn.num_profiles;i++)
    					  		{
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
			


			#ifdef DEBUG

			int i = 0, p = 0;
			for(p = 0; p < dyn.num_profiles; p++) {
				printf("\nProfile %d\n", p);
				printf("Name: %s\n", psc_data.p[p].power_stats_name);
				printf("Description: %s\n", psc_data.p[p].power_stats_descr);
				printf("Frequency: %d\n\n", psc_data.p[p].freq);
				printf("Frequency: %f\n\n", psc_data.p[p].freq_scale);
				printf("NOP Power: %f\n\n", psc_data.p[p].power[psc_data.index_nop]);
				
				fprintf(debug_file,"\nProfile %d\n", p);
				fprintf(debug_file,"Name: %s\n", psc_data.p[p].power_stats_name);
				fprintf(debug_file,"Description: %s\n", psc_data.p[p].power_stats_descr);
				fprintf(debug_file,"Frequency: %d\n\n", psc_data.p[p].freq);
				fprintf(debug_file,"Frequency: %f\n\n", psc_data.p[p].freq_scale);
				fprintf(debug_file,"NOP Power: %f\n\n", psc_data.p[p].power[psc_data.index_nop]);
			}

			printf("Instr ID | Instruction Name");
			fprintf(debug_file,"Instr ID | Instruction Name");
			for(p = 0; p < dyn.num_profiles; p++) {
				printf(" | Power Profile %d", p);
				fprintf(debug_file," | Power Profile %d", p);
			}
			printf("\n");
			fprintf(debug_file,"\n");
			for(i = 1; i <= NUM_INSTR; i++) {
				printf("%8d | %16s", i, psc_data.instr_name[i]);
				fprintf(debug_file,"%8d | %16s", i, psc_data.instr_name[i]);
				for(p = 0; p < dyn.num_profiles; p++) {
					printf(" | %15.3lf", psc_data.p[p].power[i]);
					fprintf(debug_file," | %15.3lf", psc_data.p[p].power[i]);

				}

				printf("\n");
				fprintf(debug_file,"\n");
			}

			#endif 
		}


		/****** NEW DVFS FUNCTIONS *******/
		int getPowerState() 
		{
			return dyn.actual_profile; 
		}

		void setPowerState (int state)
		{

			if (state < dyn.num_profiles)
			{
				dyn.actual_profile = state;

				update_stat_power (psc_data.index_nop, CYCLES_PER_FREQUENCY_EXCHANGE);
				
				dyn.freq_changed = true;

				#ifdef DEBUG 
				printf("\nPOWER_STATS: updating the power state to %d ",state);
				#endif
			}
		}

		bool needToWait()
		{
			if (dyn.freq_changed)
			{	
				dyn.freq_changed = false;
				return (true);
			}
			else return (false);
			

		}

		int getNumberOfStates()
		{
			return dyn.num_profiles;
		}

		void completeListOfStates (int* list)
		{

			#ifdef DEBUG
			printf("\nPOWER_STATS: returning the list of available states");
			#endif

			for (int i=0; i<dyn.num_profiles; i++)
			{
				list[i]=psc_data.p[i].freq;
			}

		}
		

		void computeRestartPower ()
		{
			update_stat_power (psc_data.index_nop, CYCLES_TO_RESTART);
		}

};
#endif

