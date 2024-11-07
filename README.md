# Measuring power draw for Xilinx devices

- `#include "vitis-power.hpp"`
- Continuously read power on one thread (see Host code below)
- Set `profile_power=true` for text file output in `power_profile.txt`.
- Compile host code with OpenMP `-fopenmp` and link `vitis-power.hpp`.

## Host code

```
  double cardPowerAvgInWatt;
  bool stop_measurement = false;
  #pragma omp parallel shared(stop_measurement, cardPowerAvgInWatt) num_threads(2)
  {
    int tid = omp_get_thread_num();
    if(tid == 0) {
        execute_on_device(copyOnEvent, kernelExecutionEvent, copyOffEvent);
    } else {
         bool profile_power=true;
         cardPowerAvgInWatt = vitis_power::U280::measureFpgaPower(stop_measurement, profile_power);
    }
    stop_measurement=true;
  }

  printf("Avg Card Power: %f W\n", cardPowerAvgInWatt);
```

## U280

`cardPowerAvgInWatt = vitis_power::U280::measureFpgaPower(stop_measurement, profile_power);`

## VCK5000

`cardPowerAvgInWatt = vitis_power::VCK5000::measureFpgaPower(stop_measurement, profile_power);`
