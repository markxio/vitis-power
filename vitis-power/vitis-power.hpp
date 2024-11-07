#include <numeric>
#include <omp.h>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <map>
#include <fstream>
#include <iostream>
#include <iomanip>

namespace vitis_power {

static std::vector<std::string> readFile(const char*); 
static void writeToFile(const char*, std::vector<double>);

class VCK5000 {
    private:
        // xbutil examine -d 0000:27:00.1 -r electrical -f JSON -o fpga_electrical.json
        // check for correct pcie ID
        inline static const std::string base="/sys/bus/pci/devices/0000:27:00.1//hwmon/hwmon7/";
        inline static const std::map<std::string, std::string> labels = {
            { "12v_pex_curr",    "curr1_input"   }, // mA 
            { "12v_aux_0_curr",  "curr2_input"   }, // mA  
            { "12v_aux_1_curr",  "curr3_input"   }, // mA  
            //{ "vccint_curr",     "curr4_input"   }, // mA

            { "12v_pex_vol",     "in0_input"     }, // mV
            //{ "3v3_pex_vol",     "in1_input"     }, // mV
            //{ "3v3_aux_vol",     "in2_input"     }, // mV
            { "12v_aux_0_vol",   "in3_input"     }, // mV  
            { "12v_aux_1_vol",   "in4_input"     }, // mV  
            //{ "vccint_vol",      "in5_input"     }, // mV 
            //{ "total_power",     "power1_input", } // W 
        }; 

    public:
        static double measureFpgaPower(bool &poison, bool isWriteToFile=false) {
            std::vector<double> card_power;
            //std::vector<double> pl_power;

            while (!poison) {
                std::map<std::string, double> vals;
                for (auto const& x : labels) {
                    //x.first x.second  
                    
                    std::string filename = base + x.second;
                    std::vector<std::string> single_element = readFile(filename.c_str());
                    //std::string key = fpga_keys[i];
                    vals[x.first] = std::stod(single_element[0]);
                }
                
                double pex = (vals["12v_pex_vol"]/1000) * (vals["12v_pex_curr"]/1000);
                double aux_0 = (vals["12v_aux_0_vol"]/1000) * (vals["12v_aux_0_curr"]/1000);
                double aux_1 = (vals["12v_aux_1_vol"]/1000) * (vals["12v_aux_0_curr"]/1000);
                //double vccint  = (vals["vccint_vol"]/1000) * (vals["vccint_curr"]/1000);
            
                card_power.push_back(pex + aux_0 + aux_1); // overall card power draw
                //pl_power.push_back(vccint); // programmable logic power draw
            }   
            double card_power_avg = accumulate( card_power.begin(), card_power.end(), 0.0)/card_power.size();
            //double pl_power_avg = accumulate( pl_power.begin(), pl_power.end(), 0.0)/pl_power.size();
            if (isWriteToFile) {
                writeToFile("power_profile_vck5000.txt", card_power); 
            }
            return card_power_avg;
        }
};

class U280 {
    public:
        static double measureFpgaPower(bool &poison, bool isWriteToFile=false) {
            // check for correct pcie ID
            std::string bus_pci="/sys/bus/pci/devices/0000:c3:00.1/xmc.u.19922944/";
            //# mV for vol / mA for curr
            std::map<std::string, double> fpga_values;
            std::vector<std::string> fpga_files;

            int n_files=4; //6;
            fpga_files.push_back("xmc_12v_aux_vol" ); 
            fpga_files.push_back("xmc_12v_pex_vol" );
            //fpga_files.push_back("xmc_vccint_vol"  );  // internal power draw for programmable logic only
            fpga_files.push_back("xmc_12v_aux_curr");
            fpga_files.push_back("xmc_12v_pex_curr");
            //fpga_files.push_back("xmc_vccint_curr" );  // internal power draw for programmable logic only
            
            std::vector<std::string> fpga_keys = fpga_files; // deep copy
            
            for (int i=0; i<n_files; i++) {
                std::stringstream ss; 
                ss << bus_pci << fpga_files[i];
                fpga_files[i] = ss.str();
            }   

            std::vector<double> card_power;
            //std::vector<double> pl_power;

            while (!poison) {
                for (int i=0; i<n_files; i++) {
                    std::vector<std::string> single_element = readFile(fpga_files[i].c_str());
                    std::string key = fpga_keys[i];
                    fpga_values[key] = std::stod(single_element[0]); // string to double
                }
                double pex = (fpga_values["xmc_12v_pex_vol"]/1000) * (fpga_values["xmc_12v_pex_curr"]/1000);
                double aux = (fpga_values["xmc_12v_aux_vol"]/1000) * (fpga_values["xmc_12v_aux_curr"]/1000);
                //double vccint  = (fpga_values["xmc_vccint_vol"]/1000) * (fpga_values["xmc_vccint_curr"]/1000);
                
                card_power.push_back(pex + aux); // overall card power draw
                //pl_power.push_back(vccint); // programmable logic power draw
            }   
            double card_power_avg = accumulate( card_power.begin(), card_power.end(), 0.0)/card_power.size();
            //double pl_power_avg = accumulate( pl_power.begin(), pl_power.end(), 0.0)/pl_power.size();
            if (isWriteToFile) {
                writeToFile("power_profile_u280.txt", card_power); 
            }            
            return card_power_avg; 
        }
};

static std::vector<std::string> readFile(const char *filename) {
    std::vector<std::string> vec;
    std::ifstream infile(filename);

    std::string line;

    if (!infile) {
        printf("### readFile() failed to read file, check your relative paths\n");
        printf("### %s\n", filename);
    }   

    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string val;

        if (!(iss >> val)) {
            fprintf(stderr, "Error, can not read file '%s'\n", filename);
            exit(-1);
        }
        vec.push_back(val);
    }   

    return vec;
}

static void writeToFile(const char *filename, std::vector<double> vec) {
    std::ofstream outputFile(filename);

    if (outputFile.is_open()) {
        for (const double& value : vec) {
            outputFile << std::fixed;
            outputFile << std::setprecision(12);
            outputFile << value << "\n";
        }
        outputFile.close();
        std::cout << "Power over time was written to power_profile.txt\n";
    }
    else {
        std::cerr << "Error opening file\n";
    }
}
}
