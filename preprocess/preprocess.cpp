#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

void Export(string name)
{
    ifstream infile(name+".spv", std::ios::binary);
    std::vector<unsigned char> data(std::istreambuf_iterator<char>(infile), {});
    infile.close();

    ofstream outfile(name+".h");
    outfile << "const unsigned char "+name+"Data[] = {";

    for (int i = 0; i < data.size(); ++i)
    {
        outfile << "0x";

        std::stringstream ss;
        ss << std::hex << (int)data[i];
        outfile << ss.str();

        if (i < data.size() - 1)
        {
            outfile << ", ";
        }
    }

    outfile << " };";
    outfile.close();
}

int main()
{
    Export("rchit");
    Export("rgen");
    Export("rint");
}
