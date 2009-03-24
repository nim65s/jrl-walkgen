/* This object generate all the values for the upperbody motion

Copyright (c) 2005-2006, 
Bjorn Verrelst,
Olivier Stasse,
Francois Keith.

JRL-Japan, CNRS/AIST

All rights reserved.
   
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:
   
* Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
* Neither the name of the CNRS/AIST nor the names of its contributors 
may be used to endorse or promote products derived from this software without specific prior written permission.
   
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS 
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER 
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <iostream>
#include <fstream>
#include <walkGenJrl/MotionGeneration/UpperBodyMotion.h>

using namespace::PatternGeneratorJRL;

UpperBodyMotion::UpperBodyMotion()
{

}

UpperBodyMotion::~UpperBodyMotion()
{
  
}



void UpperBodyMotion::GenerateDataFile(string aFileName, int LenghtDataArray)
{
  ofstream aof;

 	

  aof.open(aFileName.c_str(),ofstream::out);
  if (aof.is_open())
    {
	
      for(int i=0;i<LenghtDataArray;i++)
	{ 
	
#if 0
	  aof 		<< 0.0*M_PI/180.0 << " " << 0.0*M_PI/180.0 << " "     										//chest
			<< 0.0*M_PI/180.0 << " " << 0.0*M_PI/180.0 << " "											//head
			<< 14.813*M_PI/180.0  << " " << -10.0*M_PI/180.0 << " "	
			<< 0.0*M_PI/180.0 << " " << -30.0*M_PI/180.0 << " " 
			<< 0.0*M_PI/180.0 << " "  <<  0.0*M_PI/180.0 << " " 
			<< 10.0*M_PI/180.0 << " "	//rarm
			<< 14.813*M_PI/180.0 << " " << 10.0*M_PI/180.0 << " "	
			<< 0.0*M_PI/180.0 << " " << -30.0*M_PI/180.0 << " " 
			<< 0.0*M_PI/180.0<< " "  <<  0.0*M_PI/180.0 << " " 
			<< 10.0*M_PI/180.0<< " "	//larm
			<< -10.0*M_PI/180.0 << " " << 10.0*M_PI/180.0 << " "	
			<< -10.0*M_PI/180.0 << " " << 10.0*M_PI/180.0 << " " 
			<< -10.0*M_PI/180.0 << " "  				
	    //rhand
			<< -10.0*M_PI/180.0  << " " << 10.0*M_PI/180.0 << " "	
			<< -10.0*M_PI/180.0<< " " << 10.0*M_PI/180.0 << " " 
			<< -10.0*M_PI/180.0<< " "					
	    //lhand
			<< endl;
#else
	  aof 		<< 0.0*M_PI/180.0 << " " << 0.0*M_PI/180.0 << " "     										//chest
			<< 0.0*M_PI/180.0 << " " << 0.0*M_PI/180.0 << " "											//head
			<< 0.0*M_PI/180.0  << " " << 0.0*M_PI/180.0 << " "	
			<< 0.0*M_PI/180.0 << " " << 0.0*M_PI/180.0 << " " 
			<< 0.0*M_PI/180.0 << " "  <<  0.0*M_PI/180.0 << " " 
			<< 0.0*M_PI/180.0 << " "	//rarm
			<< 0.0*M_PI/180.0 << " " << 0.0*M_PI/180.0 << " "	
			<< 0.0*M_PI/180.0 << " " << 0.0*M_PI/180.0 << " " 
			<< 0.0*M_PI/180.0<< " "  <<  0.0*M_PI/180.0 << " " 
			<< 0.0*M_PI/180.0<< " "	//larm
			<< 0.0*M_PI/180.0 << " " << 0.0*M_PI/180.0 << " "	
			<< 0.0*M_PI/180.0 << " " << 0.0*M_PI/180.0 << " " 
			<< 0.0*M_PI/180.0 << " "  				
	    //rhand
			<< 0.0*M_PI/180.0  << " " << 0.0*M_PI/180.0 << " "	
			<< 0.0*M_PI/180.0<< " " << 0.0*M_PI/180.0 << " " 
			<< 0.0*M_PI/180.0<< " "					
	    //lhand
			<< endl;
#endif
	}
	
      aof.close();
    }
  
}


void UpperBodyMotion::ReadDataFile(string aFileName, MAL_MATRIX(&UpperBodyAngles,double))
{
 
  std::ifstream aif;

  unsigned int NumberRows, NumberColumns;
   	
  NumberRows = MAL_MATRIX_NB_ROWS(UpperBodyAngles);
  NumberColumns = MAL_MATRIX_NB_COLS(UpperBodyAngles);

  double r;

  aif.open(aFileName.c_str(),std::ifstream::in);
  if (aif.is_open())
    {
      for (unsigned int i=0;i<NumberRows;i++)
	{			
	  for (unsigned int j=0;j<NumberColumns;j++)
	    {	
	      aif >> r;	
	      UpperBodyAngles(i,j) = r;
	    }
	}		
      aif.close();
    }
  else 
    std::cerr << "UpperBodyMotion - Unable to open " << aFileName << endl;
  
}

void UpperBodyMotion::WriteDataFile(string aFileName,MAL_MATRIX( &UpperBodyAngles,double) )
{
  ofstream aof;
  unsigned int NumberRows, NumberColumns;
  NumberRows = MAL_MATRIX_NB_ROWS(UpperBodyAngles);
  NumberColumns = MAL_MATRIX_NB_COLS(UpperBodyAngles);

  aof.open(aFileName.c_str(),ofstream::out);
  if (aof.is_open())
    {
	
      for (unsigned int i=0;i<NumberRows;i++)
	{			
	  for (unsigned int j=0;j<NumberColumns;j++)
	    {	
	      aof << UpperBodyAngles(i,j)  << "\t";
	    }
	  aof << endl;
	}		
      aof.close();
    }
  
}

