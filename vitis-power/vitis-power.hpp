#include <numeric>
#include <omp.h>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <map>

namespace vitis_power {

static double measureFpgaPower(bool&);
static std::vector<std::string> readFile(const char*); 
    
static double measureFpgaPower(bool &poison) {

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
    
    return card_power_avg; 
}

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
}
