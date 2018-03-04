#include "makePlots.h"
#include <fstream>
#include <iostream>

int main(){
  TChain *chain = new TChain("HGCBD");
  string filename;
  ifstream infile("input.txt");
  infile >> filename;
  infile.close();
  if( filename.length() > 2){
    chain->Add(filename.c_str());}
  else
    cout << "There is no input root file written in the input.txt!" << endl;

  makePlots M(chain);
  M.input_RUN = filename;
  //M.Global_TS_study();
  M.Loop();
  //M.calib();
  
  return(0);
}
