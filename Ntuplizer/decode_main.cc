#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <math.h>
#include <iomanip>
#include <stdint.h>
#include "CHMapping.h"
//#include <bitset> // for cout config bits
#include <vector>
//For Ntuplizer
#include "TROOT.h"
#include "TTree.h"
#include "TFile.h"
#include <cstdint>


#include "hit_hits_class.h"
ClassImp(hit);
ClassImp(hitcollection);

//================================= Options ===================================
// Masking time sample 9 ~ 12, since they are effect by trigger signal
const bool MASKTS = true;     // Mask trigger signal
const int  EXPECTED_TS  = 13;
//=============================================================================

#define RAWSIZE 30787
#define totSCA 15  // 13SCAs + TOT + TOA
#define nSCA 13    // Real SCAs
using namespace std;

unsigned char raw[RAWSIZE];
unsigned char config[48];
unsigned int ev[4][1924];
int dati[4][128][totSCA];
int dati_ordered[4][128][totSCA];

int dac = 0;
int evt_counter;
int rollpos;
uint32_t global_TS[4];
vector<int> insert_ch;

int raw_type(const char* filename);
int decode_raw(int rawtype);
int format_channels();
int roll_position(); //Output is the location of TS 0
void globalTS();
hitcollection Fill_ntuple();
CHMapping chmap;
void read_inj_config(); //Read charge injection config



int main(){

  string logcontent;
  ifstream logfile("runinfo.log");
  while(true){

    TTree *t = new TTree("HGCBD","");
    hitcollection hits;
    t->Branch("hits",&hits);
  
    evt_counter = 0;
  
    logfile >> logcontent; // 1 file at a time
    if( logfile.eof() ) break;
    char fileName[150];
    sprintf(fileName,"%s",logcontent.c_str());
    ifstream file(fileName);

    int end = logcontent.find(".raw");
    if( end < 2 ){
      cout << endl;
      cout << "please only feed .raw data!\n" << fileName << " is not legal!\n";
      cout << endl;
      continue;}    
    
    cout << "input file: " << fileName << endl;
    int rawT = raw_type(fileName);
    if(rawT == 3) continue;

    ifstream fileinj;
    if(rawT == 0){
      // Check if injection file exist
      char fileinj_t[150];
 
      string purefname = logcontent.substr(0,end);
      sprintf(fileinj_t,"%s_Inj.txt",purefname.c_str());
      fileinj.open( fileinj_t );
      if( !fileinj.is_open() ){
	cout << "Did not find injection file " << fileinj_t
	     << ".\n Take this run as pedestal.(Inj_dac = 0)" << endl;
	dac = 0;}
      else{
	cout << "injection file: " << fileinj_t << endl;
	string dum_line;
	for(int i = 0 ; i < 5; ++i) {
	  if(i == 1) {
	    int tmp_CH;
	    fileinj >> tmp_CH;
	    insert_ch.push_back(tmp_CH);}
	  getline(fileinj,dum_line);//remove header
	}
      }
    }


    if (file.is_open()){
      int evtsize;
      switch(rawT){
      case 0: evtsize = 30787; break;
      case 1: evtsize = 30786; break;
      case 2: evtsize = 15394; break;
      default:evtsize = 30787; break;
      }
      //Remove chip config
      if(rawT == 1 || rawT == 2){
	uint8_t header[48];
	file.read( reinterpret_cast<char*>(header), 48 );
	for(int i = 0 ; i < 48 ; ++i)
	  config[i]   = header[i];
	read_inj_config(); 
      }
      //Loop event till the end of run
      while(true){
	if(evt_counter % 100 == 0)
	  cout << "processing event " << evt_counter << "..." << endl;
	uint8_t testeof[2] = {0,0};
	file.read( reinterpret_cast<char*>(testeof), 2 );
	if( file.eof() ) break;
	else{
	  raw[0] = testeof[0];
	  raw[1] = testeof[1];
	  if(rawT != 0){
	    uint8_t x[2] = {0,0};
	    for(int i = 1 ; i < evtsize/2; ++i){
	      file.read( reinterpret_cast<char*>(x), 2 );
	      raw[2*i]   = x[0];
	      raw[2*i+1] = x[1];	  }}
	  else{
	    for(int i = 2; i < evtsize; ++i)
	      file >> raw[i];}
	  
	  decode_raw(rawT);
	  format_channels();
	  rollpos = roll_position();
	  globalTS();
	  hits = Fill_ntuple();
	  int ev_dum;
	  if( rawT == 0 && fileinj.is_open() ){
	    fileinj >> ev_dum >> dac;}
	  if( rawT == 1 || rawT == 2){

	    if( (int)raw[evtsize-2] == 0xab && (int) raw[evtsize-1] == 0xcd){
	      //This is a magic number for no charge injection
	      dac = 0;}
	    else{
	      dac = (unsigned int)((raw[evtsize-1] << 8) | raw[evtsize-2]);
	      //cout << "evt = " << evt_counter <<  ", dac = " << dac << endl;
	    }
	  } 
	  t->Fill();
	  evt_counter++;
	}
      }
    }
    file.close();
    cout << "Evt = " << evt_counter << endl;

    int start   = logcontent.find_last_of("/");
    end         = logcontent.find(".raw");
    string Name = logcontent.substr(start+1,end-start-1);
    char outname[150];
    sprintf(outname,"root_out/%s.root",Name.c_str());
    cout << "output file will be: " << outname << endl;
    TFile *f = new TFile(outname,"recreate");

    t->Write();
    f->Close();
    delete t;
    
  }
  return(0);
}

int raw_type(const char* filename){
  ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  if((int)in.tellg() % 30787 == 0){
    cout << "data format of " << filename << ": old" << endl;
      return 0;}
  if( ((int)in.tellg()-48) % 30786 == 0){
    cout << "data format of " << filename << ": new" << endl;
    return 1;}
  if( ((int)in.tellg()-48) % 15394 == 0){
    cout << "data format of " << filename << ": new compressed" << endl;
    return 2;}
  else{ cout << "unrecognized data size! Rawdata may not be saved correctedly!"
	     << endl;
    return 3;}
}

void read_inj_config(){

  insert_ch.clear();
  
  unsigned char lowB,highB;
  int bitC = 0;
  int guess_CH = 0;
  bool ch_flag = false;
  for(int Q = 0; Q < 48 ; ++Q){
    lowB  = config[Q];
    highB = (lowB >> 4) & 0xf;
    lowB  = lowB & 0xf;
    for(int bit = 0; bit < 8 ; ++bit){
      int a;
      if( bit < 4 ){
	a = ( highB >> ( 3 - bit )) & 1;}
      else{
	a = ( lowB >> ( 7 - bit )) & 1;}
      if(ch_flag && guess_CH < 64) {
	if(a == 1){
	  cout << "Inj_CH = " << ( 63 - guess_CH ) << ", ";
	  insert_ch.push_back( 63 - guess_CH );
	      }
	guess_CH++;	      
      }
      bitC++;
      if(bitC >= 83 && bitC <= 147){
	ch_flag = true;}
      else
	ch_flag = false;
    }
  }
  cout << endl;
  if((int) insert_ch.size() == 0){
    cout << "Find no insertion CH -> take this run pedestal run!" << endl;
  }
  
}


int decode_raw(int rawtype){
    int i, j, k;
    unsigned char x,y;
    unsigned int t;
    unsigned int bith, bit11, bit10, bit9, bit8, bit7, bit6, bit5, bit4, bit3, bit2, bit1, bit0;
    for( i = 0; i < 1924; i = i+1){
      for (k = 0; k < 4; k = k + 1){
	ev[k][i] = 0;
      }
    }
    if(rawtype == 0 || rawtype == 1){
      int offset;
      if( rawtype == 0 ) offset = 1;
      else offset = 0;
      for( i = 0; i < 1924; i = i+1){
        for (j=0; j < 16; j = j+1){
	  x = raw[offset + i*16 + j];
	  x = x & 15;
	  for (k = 0; k < 4; k = k + 1){
	    ev[k][i] = ev[k][i] | (unsigned int) (((x >> k ) & 1) << (15 - j));
	  }
        }
      }
    }
    if(rawtype == 2){
      for( i = 0; i < 1924; i = i+1){
        for (j=0; j < 8; j = j+1){
	  x = raw[i*8 + j];
	  y = (x >> 4) & 0xf;
	  x = x & 0xf;
	  for (k = 0; k < 4; k = k + 1){
	    //ev[k][i] = ev[k][i] | (((x >> (3 - k) ) & 1) << (14 - j*2));
	    //ev[k][i] = ev[k][i] | (((y >> (3 - k) ) & 1) << (15 - j*2));
	    ev[k][i] = ev[k][i] | (uint16_t)((x >> k)  & 1) << (14 - j*2);
	    ev[k][i] = ev[k][i] | (uint16_t)((y >> k)  & 1) << (15 - j*2);
	  }
	}
      }
    }
    /*****************************************************/
    /*    Gray to binary conversion                      */
    /*****************************************************/
   	for(k = 0; k < 4 ; k = k +1 ){
   		for(i = 0; i < 128*totSCA; i = i + 1){
		 
		  bith = ev[k][i] & 0x8000;
		  t = ev[k][i] & 0x7fff;
		  bit11 = (t >> 11) & 1;
		  bit10 = bit11 ^ ((t >>10) &1);
		  bit9 = bit10 ^ ((t >>9) &1);
		  bit8 = bit9 ^ ((t >>8) &1);
		  bit7 = bit8 ^ ((t >>7) &1);
		  bit6 = bit7 ^ ((t >>6) &1);
		  bit5 = bit6 ^ ((t >>5) &1);
		  bit4 = bit5 ^ ((t >>4) &1);
		  bit3 = bit4 ^ ((t >>3) &1);
		  bit2 = bit3 ^ ((t >>2) &1);
		  bit1 = bit2 ^ ((t >>1) &1);
		  bit0 = bit1 ^ ((t >>0) &1);
		  ev[k][i] =  bith | ((bit11 << 11) + (bit10 << 10) + (bit9 << 9) + (bit8 << 8) + (bit7 << 7) + (bit6 << 6) + (bit5 << 5) + (bit4 << 4) + (bit3  << 3) + (bit2 << 2) + (bit1  << 1) + bit0);
		}
	}
	return(0);	
}

int format_channels(){
    int chip, hit, ch;
	
    for (chip =0; chip < 4; chip = chip +1 ){
        for (ch = 0; ch < 128; ch = ch +1 ){
            for (hit = 0 ; hit <totSCA ; hit = hit +1){
                dati[chip][ch][hit] = ev[chip][hit*128+ch] & 0x0FFF;
            }
        }
    }
    return(0);
}

int roll_position(){
  unsigned int roll_check;  //Just to check if 4 chip has same rollmask
  int chip,first,sec,rollpos;
  for (chip =0; chip < 4; chip = chip +1 ){
    unsigned int roll;
    roll = ev[chip][1920] & 0x1FFF;
    //cout << (int)roll << endl;
    if(chip == 0) roll_check = roll;
    else if( roll_check != roll ){
      cout << "Problematic event!( No. " << evt_counter <<") Chip" << chip
	   << "has different rollMask! Skip event!" << endl;
      return(-1);}

    unsigned char bits[13];
    first = -1; // first is actually second XD
    sec   = -1;
    for(int bit = 0; bit < 13 ; ++bit){
      bits[bit] = (roll >> bit) & 1;}
    for(int bit = 0 ; bit < 13; ++bit) {if((int)bits[bit] == 1) first = bit;}
    for(int bit = 0 ; bit < 13; ++bit) {
      if((int)bits[bit] == 1 && first != bit) sec = bit;}
    if(first == 12 && sec == 11) rollpos = 0;
    else if(first == 12 && sec == 0)  rollpos = 1;
    else rollpos = first+1;
    
    //for(int bit = 0; bit < 13 ; ++bit)      cout << (int)bits[bit] << " ";
    //cout << first << " , " << sec << ", rollpos = " << rollpos << endl;
    //getchar();
   }
  return rollpos;
}

void globalTS(){
  static const int MASK_GTS_MSB = 0x3FFF;
  static const int MASK_GTS_LSB = 0x1FFF;
  int chip;
  unsigned int t;
  unsigned int bith, bit11, bit10, bit9, bit8, bit7, bit6, bit5, bit4, bit3, bit2, bit1, bit0;
  
  for (chip =0; chip < 4; chip = chip +1 ){
    unsigned int MSB = ev[chip][1921];
    unsigned int LSB = ev[chip][1922];
    
    MSB = MSB & MASK_GTS_MSB;
    LSB = (LSB & MASK_GTS_LSB) >> 1;
    unsigned int to_binary;
    unsigned int binary_MSB;
    unsigned int binary_LSB;
    for(int ts = 1921; ts < 1923; ++ts){
      if(ts == 1921)
	to_binary = MSB;
      else
        to_binary = LSB;
      
    bith = to_binary & 0x8000;
    t = to_binary & 0x7fff;
    bit11 = (t >> 11) & 1;
    bit10 = bit11 ^ ((t >>10) &1);
    bit9 = bit10 ^ ((t >>9) &1);
    bit8 = bit9 ^ ((t >>8) &1);
    bit7 = bit8 ^ ((t >>7) &1);
    bit6 = bit7 ^ ((t >>6) &1);
    bit5 = bit6 ^ ((t >>5) &1);
    bit4 = bit5 ^ ((t >>4) &1);
    bit3 = bit4 ^ ((t >>3) &1);
    bit2 = bit3 ^ ((t >>2) &1);
    bit1 = bit2 ^ ((t >>1) &1);
    bit0 = bit1 ^ ((t >>0) &1);
    to_binary = bith | ((bit11 << 11) + (bit10 << 10) + (bit9 << 9) + (bit8 << 8) + (bit7 << 7) + (bit6 << 6) + (bit5 << 5) + (bit4 << 4) + (bit3  << 3) + (bit2 << 2) + (bit1  << 1) + bit0);
    if(ts == 1921)
      binary_MSB = to_binary;
    else
      binary_LSB = to_binary;
    }
    global_TS[chip] = (binary_MSB << 12) | binary_LSB;
    //   cout << global_TS[chip] << endl;
  }
  //getchar();
}

hitcollection Fill_ntuple(){
  hitcollection tmp_hits;

  //Make dati in ordered
  //ch 0 ~ 63: first 13 SCA HG, TOAR, TOTS
  //ch 63 ~ 127: first 13 SCA LG, TOAF, TOTF
  
  int chip, ch, sca;
  for (chip =0; chip < 4; chip = chip +1 ){
    for (ch = 0; ch < 128; ch = ch +1 ){
      for (sca = 0 ; sca < totSCA ; sca = sca +1){
	int correct_CH = 127-ch;
	dati_ordered[chip][correct_CH][sca] = dati[chip][ch][sca];
      }
    }
  }
  
  static hit H;
  bool CCorNC;
  int  chtype;
  double posx,posy;
  int TOTS,TOTF,TOAR,TOAF;
  int SCA_hg[NSCA],SCA_lg[NSCA];  
  //Hit based Fill
  int hit_counter = 0;
  for (chip =0; chip < 4; chip = chip +1 ){
    for (ch = 0; ch < 64; ch = ch +1 ){
      for (sca = 0 ; sca < NSCA ; sca = sca +1){
	//HG SCA
	SCA_hg[sca] = dati_ordered[chip][ch][sca];
	//LG SCA
	SCA_lg[sca] = dati_ordered[chip][ch+64][sca];	}
      TOAR = dati_ordered[chip][ch][13];
      TOTS = dati_ordered[chip][ch][14];
      TOAF = dati_ordered[chip][ch+64][13];
      TOTF = dati_ordered[chip][ch+64][14];
      CCorNC = (ch %2 == 0) ? true : false;
       int readposch = chip*2+ch/2;
      posx = chmap.CH_x[readposch];
      posy = chmap.CH_y[readposch];
      chtype=chmap.CH_type[readposch];

      //This 2 channel has no position on the board!
      if ( chip == 2 && ch == 60 ) {
	posx = -100;
	posy = -100;
	CCorNC = false;}
      if ( chip == 2 && ch == 61 ) {
	posx = -100;
	posy = -100;}
      
      H.Fill_hit(CCorNC,chtype,chip,ch,posx,posy,TOTS,TOTF,TOAR,TOAF,SCA_hg,SCA_lg);
      tmp_hits.Hits.push_back(H);
      hit_counter++;
    }
  }
  
  tmp_hits.evt_num = evt_counter;
  tmp_hits.hit_num = hit_counter;
  tmp_hits.inj_ch.assign(insert_ch.begin(),insert_ch.begin()+insert_ch.size());
  tmp_hits.inj_dac = dac;

  for(int i = 0 ; i < 4 ; ++i)
    tmp_hits.global_ts[i] = (int) global_TS[i];
  // BUG! conflict with framework
  for(int i = 0 ; i < NSCA; ++i){
    if(i >= rollpos)
      tmp_hits.rollposition[i] = i - rollpos;
    else
      tmp_hits.rollposition[i] = NSCA - rollpos + i; }

  // reverse
  for(int i = 0 ; i < NSCA; ++i){
    tmp_hits.rollposition[i] = 12 - tmp_hits.rollposition[i];  }  

  return tmp_hits;
}
