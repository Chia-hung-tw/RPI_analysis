#ifndef _hitandhits_
#define _hitandhits_

#include "TObject.h"
#include <vector>

const int NCHIP = 4;
const int NSCA  = 13;

class hit : public TObject {
 public:
  
  void Fill_hit(bool _CCorNC,int _chtype,int _chip,int _ch,double _posx,double _posy,int _TOTS,int _TOTF,int _TOAR,int _TOAF,int _SCA_hg[13],int _SCA_lg[13]);

  //data member ** A Hit stores all information in a single "pad" of the event
  bool CCorNC;  // Connected ch or non-connected ch (true for CC)
  int chtype;   // Cell type: 0 = Full cell , 2 = half cell,
                //            3 = Mouse bite(half), 4 = outer calib (Full Cell)
  int chip;     // Chip number for the pad. From 0 to 3
  int ch;       // Ch number for the pad. From 0 to 63
  int formatCH; // Will be calculate by (chip * 32 + ch/2 ) for CC, -1 for NC

                // * For these 4 value, only TOTs is used currently
  int TOTS;     // Time over threshold(slow), used for very high energy
  int TOTF;     // Time over threshold(fast), no one is looking at this
  int TOAR;     // Time of arrival(rise) for the pulse, a TDC information
  int TOAF;     // Time of arrival(fall) for the pulse, a TDC information

  double posx;  // position of the pad
  double posy;  // position of the pad
  int SCA_hg[NSCA]; // HighGain with 13 SCA information, used for low energy
  int SCA_lg[NSCA]; // LowGain  with 13 SCA information, used for middle energy


  ClassDef(hit,1);

};

class hitcollection : public TObject {
public:
  
  //data member
  std::vector< hit > Hits; // Vector of hits in that event
  int evt_num;             // Event number for the event
  int hit_num;             // number of hits inside the Hits vector
  std::vector<int> inj_ch; // The charge insertion channel(s), empty if ped run
  int inj_dac;             // DAC ctrl in the acquisition for the event, will be
                           // 0 if it's a pedestal run. -1 for fixed pulse ins

  int global_ts[NCHIP];    // Global time stamp, this is buggy...
                           // 4 chip sometimes have different value 

  int rollposition[NSCA]; // Which is 8 9 10 11 ... in the following example
  
                     // This rollposion look like 0 0 0 1 1 0 0 0 0 0 0 0 0
                     // Since we have 13 SCAs, the 1 1 is the trigger position.
                     // We will remove the trigeer part when analyzing data.

                     // For TS to SCA relation, we give 11 and 12 to the 2
                     // nonzero position. Then start to add from 0 to 10. EX:
                     // SCA  0  1  2  3  4  5  6  7  8  9 10 11 12
                     // roll 0  0  0  1  1  0  0  0  0  0  0  0  0
                     // TS   8  9 10 11 12  0  1  2  3  4  5  6  7

                     // We actually remove TS 9~12 in the framework since there
                     // are some study point out thet we seems to "see" trgigger
                     // in TS 9 and 10 -> by Artur

  
  ClassDef(hitcollection,2);

};

#endif
