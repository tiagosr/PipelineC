// uintN_t types for any N
#include "uintN_t.h"

// Function to test
uint1_t u24gte(uint24_t x, uint24_t y)
{
  return x >= y;
}

// Verilator device under test test bench setup
#include "../dut.h"

#ifdef __PIPELINEC__
// Define hardware debug hooks

// Generate top level debug ports
#include "debug_port.h"

// Debug ports, two inputs, one output
DEBUG_INPUT_DECL(uint24_t, x)
DEBUG_INPUT_DECL(uint24_t, y)
DEBUG_OUTPUT_DECL(uint1_t, result)
// Mark as top level for synthesis
#pragma MAIN test_bench
void test_bench()
{
  // Drive result debug port 
  // with the output of doing 
  // an operation on the two input ports
  result = u24gte(x, y);
}
#endif

// Define test params + logic using debug hooks

#define DUT_VARS_DECL \
uint24_t x;\
uint24_t y;\
uint1_t result;\
uint1_t c_result;

#define DUT_SET_NEXT_INPUTS \
if(test_num==(100000000-1))\
{\
  done = true; \
}\
/*Generate random input*/ \
x = rand() % (int)pow(2, 24); \
y = rand() % (int)pow(2, 24);

#define DUT_SET_INPUTS(top) \
DUT_SET_INPUT(top, x)\
DUT_SET_INPUT(top, y)

#define DUT_GET_OUTPUTS(top) \
DUT_GET_OUTPUT(top, result)\
c_result = u24gte(x, y);

#define DUT_COMPARE_LOG(top) \
if(c_result != result)\
{\
  DUMP_PIPELINEC_DEBUG(top) \
  cout << "FAILED c_result: " << c_result << endl;\
  test_passed = false;\
}
