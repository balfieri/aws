// main.cpp - trivial example program that would be used by a launch
//
// Just to do something silly, this program takes the instance index 
// as an argument to the program and simply computes the factorial of that
// and prints it out. Again, silly, but illustrative.
//
#include <iostream>
#include <string>

int fact( int n )
{
    int f = 1;
    while( n > 1 ) 
    {
        f *= n;
        n--;
    }
    return f;
}

int main( int argc, char * argv[] )
{
    if ( argc != 2 ) {
        std::cout << "usage: main <inst_index>\n";
        return 1;
    }
    int n = std::stoi( argv[1] );
    int f = fact( n );
    std::cout << n << "! = " << f << "\n";
    std::cout << "PASS\n";
    return 0;
}
