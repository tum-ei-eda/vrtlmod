#include "Vdiscovery.h"
#include "verilated.h"

#include <iostream>
#include <string>

int main()
{
    const std::string header_version = VERILATOR_VERSION;
    if (header_version.find(EXPECTED_VERILATOR_VERSION) != 0)
    {
        std::cerr << "Selected package and runtime headers have different versions: " << header_version << '\n';
        return 1;
    }
    Vdiscovery dut;
    dut.clk = 0;
    dut.reset = 1;
    dut.data = 0x53;
    dut.eval();
    dut.clk = 1;
    dut.eval();
    if (dut.q != 0)
        return 1;
    dut.clk = 0;
    dut.reset = 0;
    dut.eval();
    unsigned expected = 0;
    for (unsigned data : { 0U, 1U, 0x7fU, 0x80U, 0xffU })
    {
        dut.data = data;
        dut.eval();
        if (dut.q != expected)
            return 1;
        dut.clk = 1;
        dut.eval();
        expected = data ^ 0xa5U;
        if (dut.q != expected)
            return 1;
        dut.clk = 0;
        dut.eval();
        if (dut.q != expected)
            return 1;
    }
    dut.final();
    std::cout << "PASS clocked model with Verilator headers " << header_version << '\n';
    return 0;
}
