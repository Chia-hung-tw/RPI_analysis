/////////////////////////////////////////////////////////
// Arthor: Chia-hung Chien  chchien521@gmail.com       
// Just use the same class name as we used to.         
/////////////////////////////////////////////////////////

#ifndef makePlots_h
#define makePlots_h

#include "hit_hits_class.h"
#include "TChain.h"
#include "TROOT.h"
#include "TH2Poly.h"
#include "TApplication.h"
#include <string>
#include <utility> //std::pair
#include <map>     //std::map

using namespace std;
// Please Avoid variable NSCA = 13 and NCHIP = 4 defined in hit_hits_class.h
const int NCH = 64;

class makePlots{
 public:

  makePlots (TChain* inchain);
  ~makePlots();

  void Loop();
  void calib();  // This draws the calibration curve of HG,LG,TOT of the
                 // injection channel(s). Output a root file for the curves and
                 // plots for the injection channels.
  
  string         input_RUN;
  
 private:

  void Init();
  void P_and_N(int option,bool output);
  // P_and_N function:
  // option 0 can be used for pedestal run.
  // option 1 is an informal way that can deal with signal runs,
  // similar method is applied in test beam framework.
  
  void read_P_and_N(string ped_file);
  void readmap();
  void InitTH2Poly(TH2Poly& poly); //Give frame to TH2Poly

  TTree          *fChain;
  TApplication   *app;
  hitcollection  *HITS;
  hit            H;
  int            TS[NSCA];
  float avg_HG  [NCHIP][NCH][NSCA];
  float sigma_HG[NCHIP][NCH][NSCA];
  float avg_LG  [NCHIP][NCH][NSCA];
  float sigma_LG[NCHIP][NCH][NSCA];

  // map < key = chip*32+ch/2 , pair <x, y> > 
  map<int,pair < double,double > > CHmap;
};

#endif
