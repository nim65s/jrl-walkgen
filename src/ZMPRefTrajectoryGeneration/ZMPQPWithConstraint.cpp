#ifdef UNIX
#include <sys/time.h>
#endif /* UNIX */

#ifdef WIN32
#include <Windows.h>
#include <walkGenJrl/TimeUtilsWindows.h>
#endif

#include <time.h>

#include <iostream>
#include <fstream>

#include "jrlMathTools/jrlConstants.h"
#include <walkGenJrl/Mathematics/qld.h>
#include <walkGenJrl/ZMPRefTrajectoryGeneration/ZMPQPWithConstraint.h>

#if 0
#define RESETDEBUG4(y) { ofstream DebugFile; DebugFile.open(y,ofstream::out); DebugFile.close();}
#define ODEBUG4(x,y) { ofstream DebugFile; DebugFile.open(y,ofstream::app); DebugFile <<  x << endl; DebugFile.close();}
#else
#define RESETDEBUG4(y) 
#define ODEBUG4(x,y) 
#endif

#define RESETDEBUG6(y) 
#define ODEBUG6(x,y) 

#define RESETDEBUG5(y) { ofstream DebugFile; DebugFile.open(y,ofstream::out); DebugFile.close();}
#define ODEBUG5(x,y) { ofstream DebugFile; DebugFile.open(y,ofstream::app); DebugFile << x << endl; DebugFile.close();}
#define ODEBUG5NOE(x,y) { ofstream DebugFile; DebugFile.open(y,ofstream::app); DebugFile << x ; DebugFile.close();}
#if 1
#define ODEBUG(x)
#else
#define ODEBUG(x)  std::cout << "ZMPQPWithConstraint: " << x << endl;
#endif

#define ODEBUG3(x)  std::cout << "ZMPQPWithConstraint: " << x << endl;

using namespace std;
using namespace PatternGeneratorJRL;

ZMPQPWithConstraint::ZMPQPWithConstraint(SimplePluginManager *lSPM, 
					 string DataFile,
					 HumanoidSpecificities *aHS) :
  ZMPRefTrajectoryGeneration(lSPM)
{
  m_HS = aHS;
  m_ZMPD = new ZMPDiscretization(lSPM,DataFile,aHS);

  // Register method to handle
  string aMethodName[1] = 
    {":setpbwconstraint"};
  
  for(int i=0;i<1;i++)
    {
      if (!RegisterMethod(aMethodName[i]))
	{
	  std::cerr << "Unable to register " << aMethodName << std::endl;
	}
    }

  m_ConstraintOnX = 0.04;
  m_ConstraintOnY = 0.04;
  
  m_QP_T = 0.02;
  m_QP_N = 75;

  MAL_MATRIX_RESIZE(m_A,6,6);
  MAL_MATRIX_RESIZE(m_B,6,1);
  MAL_MATRIX_RESIZE(m_C,2,6);

  m_SamplingPeriod = 0.005;

}

ZMPQPWithConstraint::~ZMPQPWithConstraint()
{
  if (m_ZMPD!=0)
    delete m_ZMPD;
}


// Assuming that the points are going counter-clockwise
// and that the foot's interior is at the left of the points.
// The result is : A [ Zx(k), Zy(k)]' + B  >=0
int ZMPQPWithConstraint::ComputeLinearSystem(vector<CH_Point> aVecOfPoints, 
					     MAL_MATRIX(&A,double),
					     MAL_MATRIX(&B,double))
{
  double a,b,c;
  unsigned int n = aVecOfPoints.size();
  MAL_MATRIX_RESIZE(A,aVecOfPoints.size(),2);
  MAL_MATRIX_RESIZE(B,aVecOfPoints.size(),1);

  // Dump a file to display on scilab .
  // This should be removed during real usage inside a robot.
  if (1)
    {
      ofstream aof;
      aof.open("Constraints.dat",ofstream::app);
      for(unsigned int i=0;i<n-1;i++)
	{
	  aof << aVecOfPoints[i].col << " " <<  aVecOfPoints[i].row << " "
	      << aVecOfPoints[i+1].col << " "  << aVecOfPoints[i+1].row << endl;
	}
      aof << aVecOfPoints[n-1].col << " " <<  aVecOfPoints[n-1].row << " "
	  << aVecOfPoints[0].col << " "  << aVecOfPoints[0].row << endl;
      aof.close();
    }
  
  for(unsigned int i=0;i<n-1;i++)
    {
      
      ODEBUG("(x["<< i << "],y["<<i << "]): " << aVecOfPoints[i].col << " " <<  aVecOfPoints[i].row << " "
	     << aVecOfPoints[i+1].col << " "  << aVecOfPoints[i+1].row );

      if (fabs(aVecOfPoints[i+1].col-aVecOfPoints[i].col)>1e-7)
	{
	  double y1,x1,y2,x2,lmul=-1.0;

	  if (aVecOfPoints[i+1].col < aVecOfPoints[i].col)
	    {
	      lmul=1.0;
	      y2 = aVecOfPoints[i].row;
	      y1 = aVecOfPoints[i+1].row;
	      x2 = aVecOfPoints[i].col;
	      x1 = aVecOfPoints[i+1].col;
	    }
	  else
	    {
	      y2 = aVecOfPoints[i+1].row;
	      y1 = aVecOfPoints[i].row;
	      x2 = aVecOfPoints[i+1].col;
	      x1 = aVecOfPoints[i].col;
	    }
	  
	  
	  a = (y2 - y1)/(x2-x1) ;
	  b = (aVecOfPoints[i].row - a * aVecOfPoints[i].col);

	  a = lmul*a;
	  b = lmul*b;
	  c= -lmul;
	  

	}
      else
	{
	  c = 0.0;
	  a = -1.0;	  
	  b = aVecOfPoints[i+1].col;
	  if (aVecOfPoints[i+1].row < aVecOfPoints[i].row)
	    {
	      a=-a;
	      b=-b;
	    }
	}
      
      
      A(i,0) = a; A(i,1)= c;
      B(i,0) = b;

    }
  
  ODEBUG("(x["<< n-1 << "],y["<< n-1 << "]): " << aVecOfPoints[n-1].col << " " <<  aVecOfPoints[n-1].row << " "
	 << aVecOfPoints[0].col << " "  << aVecOfPoints[0].row );
  
  if (fabs(aVecOfPoints[0].col-aVecOfPoints[n-1].col)>1e-7)
    {
      double y1,x1,y2,x2,lmul=-1.0;
      
      if (aVecOfPoints[0].col < aVecOfPoints[n-1].col)
	{
	  lmul=1.0;
	  y2 = aVecOfPoints[n-1].row;
	  y1 = aVecOfPoints[0].row;
	  x2 = aVecOfPoints[n-1].col;
	  x1 = aVecOfPoints[0].col;
	}
      else
	{
	  y2 = aVecOfPoints[0].row;
	  y1 = aVecOfPoints[n-1].row;
	  x2 = aVecOfPoints[0].col;
	  x1 = aVecOfPoints[n-1].col;
	  
	}
      
      
      a = (y2 - y1)/(x2-x1) ;
      b = (aVecOfPoints[0].row - a * aVecOfPoints[0].col);
      
      a = lmul*a;
      b = lmul*b;
      c= -lmul;
      
    }
  else
    {
      c = 0.0;
      a = -1.0;	  
      b = aVecOfPoints[0].col;
      if (aVecOfPoints[0].row < aVecOfPoints[n-1].row)
	{
	  a=-a;
	  b=-b;
	}
    }

  
  A(n-1,0) = a; A(n-1,1)= c;
  B(n-1,0) = b;
  
  
  ODEBUG("A: " << A );
  ODEBUG("B: " << B);
      
  return 0;
}

int ZMPQPWithConstraint::BuildLinearConstraintInequalities2(deque<FootAbsolutePosition> &LeftFootAbsolutePositions,
							  deque<FootAbsolutePosition> &RightFootAbsolutePositions,
							  deque<LinearConstraintInequality_t *> &
							  QueueOfLConstraintInequalities,
							  double ConstraintOnX,
							  double ConstraintOnY)
{
  // Find the convex hull for each of the position,
  // in order to create the corresponding trajectory.
  ComputeConvexHull aCH;
  double lLeftFootHalfWidth,lLeftFootHalfHeight,
    lRightFootHalfWidth,lRightFootHalfHeight,lZ;
  
  // Read humanoid specificities.
  m_HS->GetFootSize(-1,lRightFootHalfWidth,lRightFootHalfHeight,lZ);
  m_HS->GetFootSize(1,lLeftFootHalfWidth,lLeftFootHalfHeight,lZ);
  
  lRightFootHalfWidth *= 0.5;
  lRightFootHalfHeight *= 0.5;
  lLeftFootHalfWidth *= 0.5;
  lLeftFootHalfHeight *= 0.5;

  lLeftFootHalfHeight -= ConstraintOnY;
  lRightFootHalfHeight -= ConstraintOnY;

  lLeftFootHalfWidth -= ConstraintOnX;
  lRightFootHalfWidth -= ConstraintOnX;
  
  if (LeftFootAbsolutePositions.size()!=
      RightFootAbsolutePositions.size())
    return -1;
  
  int State=0; // State for the system 0:start, 1: Right Support Foot, 2: Left Support Foot,
  // 3: Double Support.
  int ComputeCH=0;
  float lx=0.0, ly=0.0;
  float lxcoefs[4] = { 1.0, 1.0, -1.0, -1.0};
  float lycoefs[4] = {-1.0, 1.0,  1.0, -1.0};


  // Going through the set of generated data for each 5 ms.
  // from this extract a set of linear constraints.
  for(unsigned int i=0;i<LeftFootAbsolutePositions.size();i++)
    {
      
      ComputeCH=0;
      // First check if we have to compute a convex hull
      if (i==0)
	{
	  ComputeCH = 1;
	  State=3;
	}
      // Double support
      if (LeftFootAbsolutePositions[i].stepType>=10)
	{
	  if (State!=3)
	    ComputeCH=1;
	  State =3;
	}
      else
	{
	  if (LeftFootAbsolutePositions[i].z>0)
	    {
	      if (State!=2)
		ComputeCH=1;
	      State=2;
	    }
	  else if (RightFootAbsolutePositions[i].z>0)
	    {
	      if (State!=1)
		ComputeCH=1;
	      State=1;
	    }
	  
	}

      if (ComputeCH)
	{
	  vector<CH_Point> TheConvexHull;
	  // Check if we are in a single or double support phase,
	  // by testing the step type. In double support phase
	  // the value is greater than or equal to 10.
	  if (State==3)
	    {
	      // In this case the convex hull 
	      TheConvexHull.resize(4);

		      
	      if (LeftFootAbsolutePositions[i].x<RightFootAbsolutePositions[i].x)
		{
		  TheConvexHull[0].col= RightFootAbsolutePositions[i].x +  lRightFootHalfWidth;
		  TheConvexHull[1].col= RightFootAbsolutePositions[i].x +  lRightFootHalfWidth;
		  TheConvexHull[2].col= LeftFootAbsolutePositions[i].x -  lLeftFootHalfWidth;
		  TheConvexHull[3].col= LeftFootAbsolutePositions[i].x -  lLeftFootHalfWidth;
		  
		}
	      else 
		{
		  TheConvexHull[0].col= LeftFootAbsolutePositions[i].x +  lLeftFootHalfWidth;
		  TheConvexHull[1].col= LeftFootAbsolutePositions[i].x +  lLeftFootHalfWidth;
		  TheConvexHull[2].col= RightFootAbsolutePositions[i].x -  lRightFootHalfWidth;
		  TheConvexHull[3].col= RightFootAbsolutePositions[i].x -  lRightFootHalfWidth;

		}


	      if (LeftFootAbsolutePositions[i].y<RightFootAbsolutePositions[i].y)
		{
		  TheConvexHull[0].row= LeftFootAbsolutePositions[i].y - lLeftFootHalfHeight;
		  TheConvexHull[1].row= RightFootAbsolutePositions[i].y +  lRightFootHalfHeight;
		  TheConvexHull[2].row= RightFootAbsolutePositions[i].y +  lRightFootHalfHeight;
		  TheConvexHull[3].row= LeftFootAbsolutePositions[i].y -  lLeftFootHalfHeight;
		  
		}
	      else 
		{
		  TheConvexHull[0].row= RightFootAbsolutePositions[i].y -  lRightFootHalfHeight;
		  TheConvexHull[1].row= LeftFootAbsolutePositions[i].y +  lLeftFootHalfHeight;
		  TheConvexHull[2].row= LeftFootAbsolutePositions[i].y +  lLeftFootHalfHeight;
		  TheConvexHull[3].row= RightFootAbsolutePositions[i].y -  lRightFootHalfHeight;

		}

	    }
	  // In the second case, it is necessary to compute 
	  // the support foot.
	  else
	    {
	      
	      TheConvexHull.resize(4);
	      
	      // Who is support foot ?
	      if (LeftFootAbsolutePositions[i].z < RightFootAbsolutePositions[i].z)
		{
		  lx=LeftFootAbsolutePositions[i].x;
		  ly=LeftFootAbsolutePositions[i].y;
	      
		  for(unsigned j=0;j<4;j++)
		    {
		      TheConvexHull[j].col = lx + lxcoefs[j]*  lLeftFootHalfWidth;
		      TheConvexHull[j].row = ly + lycoefs[j]*  lLeftFootHalfHeight;
		    }
			      
		}
	      else
		{
		  lx=RightFootAbsolutePositions[i].x;
		  ly=RightFootAbsolutePositions[i].y;
		  
		  for(unsigned j=0;j<4;j++)
		    {
		      TheConvexHull[j].col = lx + lxcoefs[j]*  lRightFootHalfWidth;
		      TheConvexHull[j].row = ly + lycoefs[j]*  lRightFootHalfHeight;
		    }
		}
	      
	      
	    }

	  // Linear Constraint Inequality
	  LinearConstraintInequality_t * aLCI = new LinearConstraintInequality_t;
	  ComputeLinearSystem(TheConvexHull,aLCI->A, aLCI->B);
	  aLCI->StartingTime = LeftFootAbsolutePositions[i].time;
	  if (QueueOfLConstraintInequalities.size()>0)
	    {
	      QueueOfLConstraintInequalities.back()->EndingTime = LeftFootAbsolutePositions[i].time;

	    }

	  QueueOfLConstraintInequalities.push_back(aLCI);
	  
	  
	  
	}
      if (i==LeftFootAbsolutePositions.size()-1)
	{
	  if (QueueOfLConstraintInequalities.size()>0)
	    {
	      QueueOfLConstraintInequalities.back()->EndingTime = LeftFootAbsolutePositions[i].time;
	    }
	}
    }

  ODEBUG("Size of the 5 ms array: "<< LeftFootAbsolutePositions.size());
  ODEBUG("Size of the queue of Linear Constraint Inequalities " << QueueOfLConstraintInequalities.size());
  
  return 0;
}

int ZMPQPWithConstraint::BuildLinearConstraintInequalities(deque<FootAbsolutePosition> &LeftFootAbsolutePositions,
							 deque<FootAbsolutePosition> &RightFootAbsolutePositions,
							 deque<LinearConstraintInequality_t *> &
							 QueueOfLConstraintInequalities,
							 double ConstraintOnX,
							 double ConstraintOnY)
{
  // Find the convex hull for each of the position,
  // in order to create the corresponding trajectory.
  ComputeConvexHull aCH;
  double lLeftFootHalfWidth,lLeftFootHalfHeight,
    lRightFootHalfWidth,lRightFootHalfHeight,lZ;
  
  // Read humanoid specificities.
  m_HS->GetFootSize(-1,lRightFootHalfWidth,lRightFootHalfHeight,lZ);
  m_HS->GetFootSize(1,lLeftFootHalfWidth,lLeftFootHalfHeight,lZ);

  lRightFootHalfWidth *= 0.5;
  lRightFootHalfHeight *= 0.5;
  lLeftFootHalfWidth *= 0.5;
  lLeftFootHalfHeight *= 0.5;
  
  lLeftFootHalfHeight -= ConstraintOnY;
  lRightFootHalfHeight -= ConstraintOnY;

  lLeftFootHalfWidth -= ConstraintOnX;
  lRightFootHalfWidth -= ConstraintOnX;
  
  if (LeftFootAbsolutePositions.size()!=
      RightFootAbsolutePositions.size())
    return -1;
  
  int State=0; // State for the system 0:start, 1: Right Support Foot, 2: Left Support Foot,
  // 3: Double Support.
  int ComputeCH=0;
  float lx=0.0, ly=0.0;
  float lxcoefs[4] = { 1.0, 1.0, -1.0, -1.0};
  float lycoefs[4] = {-1.0, 1.0,  1.0, -1.0};
  double prev_xmin=1e7, prev_xmax=-1e7, prev_ymin=1e7, prev_ymax=-1e7;
  RESETDEBUG4("ConstraintMax.dat");

  double s_t,c_t;

  // Going through the set of generated data for each 5 ms.
  // from this extract a set of linear constraints.
  for(unsigned int i=0;i<LeftFootAbsolutePositions.size();i++)
    {
      
      ComputeCH=0;
      // First check if we have to compute a convex hull
      if (i==0)
	{
	  ComputeCH = 1;
	  State=3;
	}
      // Double support
      if (LeftFootAbsolutePositions[i].stepType>=10)
	{
	  if (State!=3)
	    ComputeCH=1;
	  State =3;
	}
      else
	{
	  
	  if (LeftFootAbsolutePositions[i].z>0)
	    {
	      if (State!=2)
		ComputeCH=1;
	      State=2;
	    }
	  else if (RightFootAbsolutePositions[i].z>0)
	    {
	      if (State!=1)
		ComputeCH=1;
	      State=1;
	    }
	  else if ((RightFootAbsolutePositions[i].z==0.0) &&
		   (LeftFootAbsolutePositions[i].z==0.0))
	    {
	      if (State!=3)
		ComputeCH=1;
	      State=3;
	    } 

	  
	}

      if (ComputeCH)
	{
	  double xmin=1e7, xmax=-1e7, ymin=1e7, ymax=-1e7;

	  ODEBUG("LeftFootAbsolutePositions[" << i << " ].theta= " << 
		  LeftFootAbsolutePositions[i].theta);

	  ODEBUG("RightFootAbsolutePositions[" << i << " ].theta= " << 
		  RightFootAbsolutePositions[i].theta);
	      
	  vector<CH_Point> TheConvexHull;
	  // Check if we are in a single or double support phase,
	  // by testing the step type. In double support phase
	  // the value is greater than or equal to 10.
	  // In this case, we have to compute the convex hull
	  // of both feet.
	  if (State==3)
	    {
	      vector<CH_Point> aVecOfPoints;
	      
	      aVecOfPoints.resize(8);

	      lx=LeftFootAbsolutePositions[i].x;
	      ly=LeftFootAbsolutePositions[i].y;
	      
	      s_t = sin(LeftFootAbsolutePositions[i].theta*M_PI/180.0); 
	      c_t = cos(LeftFootAbsolutePositions[i].theta*M_PI/180.0);  
	      for(unsigned j=0;j<4;j++)
		{
		  aVecOfPoints[j].col = lx + ( lxcoefs[j] * lLeftFootHalfWidth 
					       * c_t 
					       - lycoefs[j] * 
					       lLeftFootHalfHeight * s_t );
		  aVecOfPoints[j].row = ly + ( lxcoefs[j] * lLeftFootHalfWidth 
					       * s_t 
					       + lycoefs[j] * 
					       lLeftFootHalfHeight * c_t );


		  // Computes the maxima.
		  xmin = aVecOfPoints[j].col < xmin ? aVecOfPoints[j].col : xmin;
		  xmax = aVecOfPoints[j].col > xmax ? aVecOfPoints[j].col : xmax;
		  ymin = aVecOfPoints[j].row < ymin ? aVecOfPoints[j].row : ymin;
		  ymax = aVecOfPoints[j].row > ymax ? aVecOfPoints[j].row : ymax;
		  
		}
	      ODEBUG("State 3-1 " << xmin << " " << xmax << " " << ymin << " " << ymax);
	      lx=RightFootAbsolutePositions[i].x;
	      ly=RightFootAbsolutePositions[i].y;

	      
	      s_t = sin(RightFootAbsolutePositions[i].theta*M_PI/180.0); //+
	      c_t = cos(RightFootAbsolutePositions[i].theta*M_PI/180.0); //+ 
	      
	      ODEBUG("Right Foot: " << lx << " " << ly << " " << RightFootAbsolutePositions[i].theta);
	      for(unsigned j=0;j<4;j++)
		{
		  aVecOfPoints[j+4].col = lx + ( lxcoefs[j] * lRightFootHalfWidth
						 * c_t - lycoefs[j] * 
						 lRightFootHalfHeight * s_t ); 
		  aVecOfPoints[j+4].row = ly + ( lxcoefs[j] * lRightFootHalfWidth
						 * s_t + lycoefs[j] * 
						 lRightFootHalfHeight * c_t );  
		  // Computes the maxima.
		  xmin = aVecOfPoints[j+4].col < xmin ? aVecOfPoints[j+4].col : xmin;
		  xmax = aVecOfPoints[j+4].col > xmax ? aVecOfPoints[j+4].col : xmax;
		  ymin = aVecOfPoints[j+4].row < ymin ? aVecOfPoints[j+4].row : ymin;
		  ymax = aVecOfPoints[j+4].row > ymax ? aVecOfPoints[j+4].row : ymax;
		  
		}

	      ODEBUG("State 3-2" << xmin << " " << xmax << " " << ymin << " " << ymax);
	      aCH.DoComputeConvexHull(aVecOfPoints,TheConvexHull);
	    }
	  // In the second case, it is necessary to compute 
	  // the support foot.
	  else
	    {
	      
	      TheConvexHull.resize(4);
	      
	      // Who is support foot ?
	      if (LeftFootAbsolutePositions[i].z < RightFootAbsolutePositions[i].z)
		{
		  lx=LeftFootAbsolutePositions[i].x;
		  ly=LeftFootAbsolutePositions[i].y;
		  
		  s_t = sin(LeftFootAbsolutePositions[i].theta*M_PI/180.0); 
		  c_t = cos(LeftFootAbsolutePositions[i].theta*M_PI/180.0);
 		  for(unsigned j=0;j<4;j++)
		    {
		      TheConvexHull[j].col = lx + 
			( lxcoefs[j] * lLeftFootHalfWidth * c_t -
			  lycoefs[j] * lLeftFootHalfHeight * s_t ); 
		      TheConvexHull[j].row = ly + 
			( lxcoefs[j] * lLeftFootHalfWidth * s_t + 
			  lycoefs[j] * lLeftFootHalfHeight * c_t ); 
		      // Computes the maxima.
		      xmin = TheConvexHull[j].col < xmin ? TheConvexHull[j].col : xmin;
		      xmax = TheConvexHull[j].col > xmax ? TheConvexHull[j].col : xmax;
		      ymin = TheConvexHull[j].row < ymin ? TheConvexHull[j].row : ymin;
		      ymax = TheConvexHull[j].row > ymax ? TheConvexHull[j].row : ymax;

		    }
		  ODEBUG("Left support foot");
		}
	      else
		{
		  lx=RightFootAbsolutePositions[i].x;
		  ly=RightFootAbsolutePositions[i].y;
		  s_t = sin(RightFootAbsolutePositions[i].theta*M_PI/180.0); 
		  c_t = cos(RightFootAbsolutePositions[i].theta*M_PI/180.0);      
		  for(unsigned j=0;j<4;j++)
		    {
		      TheConvexHull[j].col = lx + ( lxcoefs[j] * 
						    lRightFootHalfWidth * c_t -
						    lycoefs[j] * 
						    lRightFootHalfHeight * s_t );
		      TheConvexHull[j].row = ly + ( lxcoefs[j] * 
						    lRightFootHalfWidth * s_t +
						    lycoefs[j] * 
						    lRightFootHalfHeight * c_t ); 
		      // Computes the maxima.
		      xmin = TheConvexHull[j].col < xmin ? TheConvexHull[j].col : xmin;
		      xmax = TheConvexHull[j].col > xmax ? TheConvexHull[j].col : xmax;
		      ymin = TheConvexHull[j].row < ymin ? TheConvexHull[j].row : ymin;
		      ymax = TheConvexHull[j].row > ymax ? TheConvexHull[j].row : ymax;

		    }
		  ODEBUG("Right support foot");
		}
	      ODEBUG("State !=3 " << xmin << " " << xmax << " " << ymin << " " << ymax);
	      
	    }

	  // Linear Constraint Inequality
	  LinearConstraintInequality_t * aLCI = new LinearConstraintInequality_t;
	  ComputeLinearSystem(TheConvexHull,aLCI->A, aLCI->B);
	  aLCI->StartingTime = LeftFootAbsolutePositions[i].time;
	  if (QueueOfLConstraintInequalities.size()>0)
	    {
	      QueueOfLConstraintInequalities.back()->EndingTime = LeftFootAbsolutePositions[i].time;
	      ODEBUG4( QueueOfLConstraintInequalities.back()->StartingTime << " " <<
		       QueueOfLConstraintInequalities.back()->EndingTime << " " <<
		       prev_xmin << " "  <<
		       prev_xmax << " "  <<
		       prev_ymin << " "  <<
		       prev_ymax
		       ,"ConstraintMax.dat");

	    }
	  ODEBUG("Final " << xmin << " " << xmax << " " << ymin << " " << ymax);
	  prev_xmin = xmin; prev_xmax = xmax;
	  prev_ymin = ymin; prev_ymax = ymax;

	  QueueOfLConstraintInequalities.push_back(aLCI);
	  
	  
	  
	}
      if (i==LeftFootAbsolutePositions.size()-1)
	{
	  if (QueueOfLConstraintInequalities.size()>0)
	    {
	      QueueOfLConstraintInequalities.back()->EndingTime = LeftFootAbsolutePositions[i].time;
	      ODEBUG4( QueueOfLConstraintInequalities.back()->StartingTime << " " <<
		       QueueOfLConstraintInequalities.back()->EndingTime << " " <<
		       prev_xmin << " "  <<
		       prev_xmax << " "  <<
		       prev_ymin << " "  <<
		       prev_ymax
		       ,"ConstraintMax.dat");

	    }
	}
    }

  ODEBUG("Size of the 5 ms array: "<< LeftFootAbsolutePositions.size());
  ODEBUG("Size of the queue of Linear Constraint Inequalities " << QueueOfLConstraintInequalities.size());
  
  return 0;
}

int ZMPQPWithConstraint::BuildMatricesPxPu(double * & Px,double * &Pu, 
					 unsigned N, double T,
					 double StartingTime,
					 deque<LinearConstraintInequality_t *> & QueueOfLConstraintInequalities,
					 double Com_Height,
					 unsigned int &NbOfConstraints,
					 MAL_VECTOR(& xk,double))
{
  // Discretize the problem.
  ODEBUG(" N:" << N << " T: " << T);
  
  // Creates the matrices.
  // The memory will be bounded to 8 constraints per
  // support foot (double support case).
  // Will be probably all the time smaller.
  if (Px==0)
    Px = new double[8*N+1];

  if (Pu==0)
    Pu = new double[(8*N+1)*2*N];

  memset(Pu,0,(8*N+1)*2*N*sizeof(double));
  //memset(Px,0,(8*N+1)*sizeof(double));

  deque<LinearConstraintInequality_t *>::iterator LCI_it, store_it;
  LCI_it = QueueOfLConstraintInequalities.begin();
  while (LCI_it!=QueueOfLConstraintInequalities.end())
    {
      if ((StartingTime>=(*LCI_it)->StartingTime) &&
	  (StartingTime<=(*LCI_it)->EndingTime))
	{
	  break;
	}
      LCI_it++;
    }
  store_it = LCI_it;
  

  // Did not find the appropriate Linear Constraint.
  if (LCI_it==QueueOfLConstraintInequalities.end())
    {
      cout << "HERE 3" << endl;
      return -1;
    }
      
  if (0)
    {
      char Buffer[1024];
      sprintf(Buffer,"PXD_%f.dat", StartingTime);
      RESETDEBUG4(Buffer);
      ODEBUG6("xk:" << xk << " Starting time: " <<StartingTime ,Buffer );
      char Buffer2[1024];
      sprintf(Buffer2,"PXxD_%f.dat", StartingTime);
      RESETDEBUG4(Buffer2);
      
      char Buffer3[1024];
      sprintf(Buffer3,"PXyD_%f.dat", StartingTime);
      RESETDEBUG4(Buffer3);
    }
  
  // Compute first the number of constraint.
  unsigned int IndexConstraint=0;
  for(unsigned int i=0;i<N;i++)
    {

      double ltime = StartingTime+ i* T;
      if (ltime > (*LCI_it)->EndingTime)
	LCI_it++;

      if (LCI_it==QueueOfLConstraintInequalities.end())
	{
	  break;
	}
      IndexConstraint += MAL_MATRIX_NB_ROWS((*LCI_it)->A);
    }  
  NbOfConstraints = IndexConstraint;

  LCI_it = store_it;
  IndexConstraint = 0;
  ODEBUG("Starting Matrix to build the constraints. ");
  ODEBUG((*LCI_it)->A );
  for(unsigned int i=0;i<N;i++)
    {

      double ltime = StartingTime+ i* T;
      if (ltime > (*LCI_it)->EndingTime)
	{
	  LCI_it++;
	}

      if (LCI_it==QueueOfLConstraintInequalities.end())
	{
	}

      // For each constraint.
      for(unsigned j=0;j<MAL_MATRIX_NB_ROWS((*LCI_it)->A);j++)
	{
	  Px[IndexConstraint] = 
	    // X Axis * A
	    (xk[0] +
	     xk[1] * T *(i+1) + 
	     xk[2]*((i+1)*(i+1)*T*T/2 - Com_Height/9.81))
	    * (*LCI_it)->A(j,0)
	     + 
	     // Y Axis * A
	    ( xk[3]+ xk[4]* T * (i+1) + 
	       xk[5]*((i+1)*(i+1)*T*T/2 - Com_Height/9.81))	  
	    * (*LCI_it)->A(j,1)
	     // Constante part of the constraint
	    + (*LCI_it)->B(j,0);

	  ODEBUG6(Px[IndexConstraint] << " " << (*LCI_it)->A(j,0)  << " "
		  << (*LCI_it)->A[j][1] << " " << (*LCI_it)->B(j,0) ,Buffer);
	  ODEBUG6(1 << " " <<    T *(i+1) << " " <<    (i+1)*(i+1)*T*T/2 - Com_Height/9.81,Buffer2);
	  ODEBUG6(1 << " " <<    T *(i+1) << " " <<    (i+1)*(i+1)*T*T/2 - Com_Height/9.81,Buffer3);
	  for(unsigned k=0;k<=i;k++)
	    {
	      // X axis
	      Pu[IndexConstraint+k*(NbOfConstraints+1)] = 
		(*LCI_it)->A(j,0)*
		((1+3*(i-k)+3*(i-k)*(i-k))*T*T*T/6.0 - T * Com_Height/9.81);
	      
	      // Y axis
	      Pu[IndexConstraint+(k+N)*(NbOfConstraints+1)] = 
		(*LCI_it)->A(j,1)*
		((1+3*(i-k)+3*(i-k)*(i-k))*T*T*T/6.0 - T * Com_Height/9.81);
		 
	      
	    }
	  ODEBUG("IC: " << IndexConstraint );
	  IndexConstraint++;
	}

    }
  ODEBUG6("Index Constraint :"<< IndexConstraint,Buffer);
  if (0)
    {
      ofstream aof;
      char Buffer[1024];
      sprintf(Buffer,"Pu_%f.dat", StartingTime);
      aof.open(Buffer,ofstream::out);
      for(unsigned int i=0;i<IndexConstraint;i++)
	{
	  for(unsigned int j=0;j<2*N;j++)
	    aof << Pu[i+j*(NbOfConstraints+1)] << " " ;
	  aof << endl;
	}
      aof.close();
      
      sprintf(Buffer,"PX_%f.dat", StartingTime);
      aof.open(Buffer,ofstream::out);
      for(unsigned int i=0;i<IndexConstraint;i++)
	{
	  aof << Px[i] << endl ;
	}
      aof.close();
    }

  return 0;
}

int ZMPQPWithConstraint::BuildZMPTrajectoryFromFootTrajectory(deque<FootAbsolutePosition> &LeftFootAbsolutePositions,
							    deque<FootAbsolutePosition> &RightFootAbsolutePositions,
							    deque<ZMPPosition> &ZMPRefPositions,
							    deque<ZMPPosition> &NewFinalZMPPositions,
							    deque<COMPosition> &COMPositions,
							    double ConstraintOnX,
							    double ConstraintOnY,
							    double T,
							    unsigned int N)
{
  //  double T=0.02; 
  //double T=0.02;
  //  unsigned int N=75; 
  //  unsigned int N = 100;
  //  double ComHeight=0.814;
  double ComHeight=0.80;
  double *Px=0,*Pu=0;
  unsigned int NbOfConstraints=8*N; // Nb of constraints to be taken into account
  // for each iteration
  MAL_VECTOR(xk,double);MAL_VECTOR(Buk,double);MAL_VECTOR(zk,double);
  MAL_MATRIX(vnlPx,double); MAL_MATRIX(vnlPu,double);
  MAL_MATRIX(vnlValConstraint,double);
  MAL_MATRIX(vnlX,double);MAL_MATRIX(vnlStorePx,double);
  MAL_MATRIX(vnlStoreX,double);
  MAL_VECTOR(ConstraintNb,int);
  MAL_MATRIX(PPu,double); MAL_MATRIX(VPu,double); 
  MAL_MATRIX(VPx,double); MAL_MATRIX(PPx,double);
  MAL_MATRIX(Id,double);MAL_MATRIX(OptA,double);
  MAL_MATRIX(OptB,double);MAL_MATRIX( OptC,double);
  MAL_VECTOR(ZMPRef,double);MAL_VECTOR(OptD,double);
  double alpha = 200.0, beta = 1000.0;
  int CriteriaToMaximize=1;


  RESETDEBUG4("DebugInterpol.dat");
  
  MAL_MATRIX_RESIZE(PPu,2*N,2*N);
  MAL_MATRIX_RESIZE(VPu,2*N,2*N);
  MAL_MATRIX_RESIZE(PPx,2*N,6);
  MAL_MATRIX_RESIZE(VPx,2*N,6);
  MAL_MATRIX_RESIZE(Id,2*N,2*N);
  MAL_VECTOR_RESIZE(ZMPRef,2*N);

  for(unsigned int i=0;i<N;i++)
    {
      // Compute VPx and PPx
      VPx(i,0)   = 0.0;   VPx(i,1) =     1.0; VPx(i,2)   = (i+1)*T;
      VPx(i,3)   = 0.0;   VPx(i,4) =     0.0; VPx(i,5)   = 0.0;
      VPx(i+N,0) = 0.0;   VPx(i+N,1) =   0.0; VPx(i+N,2) = 0.0;
      VPx(i+N,3) = 0.0;   VPx(i+N,4) =   1.0; VPx(i+N,5) = (i+1)*T;

      PPx(i,0) = 1.0; PPx(i,1)     = (i+1)*T; PPx(i,2) = (i+1)*(i+1)*T*T*0.5;
      PPx(i,3) = 0.0; PPx(i,4)     =       0; PPx(i,5) = 0.;
      PPx(i+N,0) = 0.0; PPx(i+N,1) =     0.0; PPx(i+N,2) = 0.0;
      PPx(i+N,3) = 1.0; PPx(i+N,4) = (i+1)*T; PPx(i+N,5) = (i+1)*(i+1)*T*T*0.5;
      
      
      for(unsigned int j=0;j<N;j++)
	{
	  PPu(i,j)=0;
	  
	  if (j<=i)
	    {

	      VPu(i,j)= (2*(i-j)+1)*T*T*0.5 ;
	      VPu(i+N,j+N)= (2*(i-j)+1)*T*T*0.5 ;
	      VPu(i,j+N)=0.0;
	      VPu(i+N,j)=0.0;

	      PPu(i,j)= (1 + 3*(i-j) + 3*(i-j)*(i-j)) * T*T*T/6.0;
	      PPu(i+N,j+N)= (1 + 3*(i-j) + 3*(i-j)*(i-j)) * T*T*T/6.0;
	      PPu(i,j+N)=0.0;
	      PPu(i+N,j)=0.0;

	    }
	  else
	    {

	      VPu(i,j) = 0.0;
	      VPu(i+N,j+N)=0.0;
	      VPu(i,j+N)=0.0;
	      VPu(i+N,j)=0.0;

	      PPu(i,j) = 0.0;
	      PPu(i+N,j+N)=0.0;
	      PPu(i,j+N)=0.0;
	      PPu(i+N,j)=0.0;

	    }

	  // Identity.
	  if (i==j)
	    {
	      Id(i,j)=1.0;Id(i+N,j+N)=1.0;
	      Id(i+N,j)=0.0;Id(i,j+N)=0.0;
	    }
	  else
	    {
	      Id(i,j)=0.0;Id(i+N,j+N)=0.0;
	      Id(i+N,j)=0.0;Id(i,j+N)=0.0;
	    }

	  //	  Zeros(i,j)=0.0;Zeros(i+N,j+N)=0.0;
	  //	  Zeros(i+N,j)=0.0;Zeros(i,j+N)=0.0;

	}
    }

  if (0)
    {
      ofstream aof;
      aof.open("VPx.dat");
      aof << VPx;
      aof.close();
      
      aof.open("PPx.dat");
      aof << PPx;
      aof.close();
      
      aof.open("VPu.dat");
      aof << VPu;
      aof.close();
      
      aof.open("PPu.dat");
      aof << PPu;
      aof.close();
    }
  
  MAL_MATRIX_RESIZE(vnlX,2*N,1);
  
  MAL_VECTOR_RESIZE(xk,6);
  for(unsigned int i=0;i<6;i++)
    xk[i] = 0.0;
  MAL_VECTOR_RESIZE(Buk,6);
  MAL_VECTOR_RESIZE(zk,2);

  int m = NbOfConstraints;
  int me= 0;
  int mmax = NbOfConstraints+1;
  int n = 2*N;
  int nmax = 2*N; // Size of the matrix to compute the cost function.
  int mnn = m+n+n;

  double *C=new double[4*N*N]; // Objective function matrix
  double *D=new double[2*N];   // Constant part of the objective function
  double *XL=new double[2*N];  // Lower bound of the jerk.
  double *XU=new double[2*N];  // Upper bound of the jerk.
  double *X=new double[2*N];   // Solution of the system.
  double Eps=1e-8 ;
  double *U = (double *)malloc( sizeof(double)*mnn); // Returns the Lagrange multipliers.;

  // Initialization of the matrices
  memset(C,0,4*N*N*sizeof(double));
  for(unsigned int i=0;i<2*N;i++)
    C[i*2*N+i] = 1.0;
  
  int iout=0;
  int ifail;
  int iprint=1;
  int lwar=3*nmax*nmax/2+ 10*nmax  + 2*mmax + 20000;;
  double *war= (double *)malloc(sizeof(double)*lwar);
  int liwar = n; //
  int *iwar = new int[liwar]; // The Cholesky decomposition is done internally.


  for(int i=0;i<6;i++)
    {
      m_B(i,0) = 0.0;
      m_C(0,i) = 0.0;
      m_C(1,i) = 0.0;
      for(int j=0;j<6;j++)
	m_A(i,j)=0.0;
    }

  m_A(0,0) = 1.0; m_A(0,1) =   T; m_A(0,2) = T*T/2.0;
  m_A(1,0) = 0.0; m_A(1,1) = 1.0; m_A(1,2) = T;
  m_A(2,0) = 0.0; m_A(2,1) = 0.0; m_A(2,2) = 1.0;
  m_A(3,3) = 1.0; m_A(3,4) =   T; m_A(3,5) = T*T/2.0;
  m_A(4,3) = 0.0; m_A(4,4) = 1.0; m_A(4,5) = T;
  m_A(5,3) = 0.0; m_A(5,4) = 0.0; m_A(5,5) = 1.0;
  

  m_B(0,0) = T*T*T/6.0;
  m_B(1,0) = T*T/2.0;
  m_B(2,0) = T;
  m_B(3,0) = T*T*T/6.0;
  m_B(4,0) = T*T/2.0;
  m_B(5,0) = T;

  
  m_C(0,0) = 1.0;
  m_C(0,1) = 0.0;
  m_C(0,2) = -ComHeight/9.81;

  m_C(1,3) = 1.0;
  m_C(1,4) = 0.0;
  m_C(1,5) = -ComHeight/9.81;

  deque<LinearConstraintInequality_t *> QueueOfLConstraintInequalities;
  
  if (1)
    {
      RESETDEBUG4("DebugPBW.dat");
      RESETDEBUG4("DebugPBW_Pb.dat");

      ODEBUG6("A:" << m_A << endl << "B:" << m_B, "DebugPBW_Pb.dat");
  

      RESETDEBUG5("Constraints.dat");

    }
      
  // Build a set of linear constraint inequalities.
  BuildLinearConstraintInequalities(LeftFootAbsolutePositions,
				    RightFootAbsolutePositions,
				    QueueOfLConstraintInequalities,
				    ConstraintOnX,
				    ConstraintOnY);

  deque<LinearConstraintInequality_t *>::iterator LCI_it;
  LCI_it = QueueOfLConstraintInequalities.begin();
  while(LCI_it!=QueueOfLConstraintInequalities.end())
    {
      //      cout << *LCI_it << endl; 
      //      cout << (*LCI_it)->StartingTime << " " << (*LCI_it)->EndingTime << endl;
      LCI_it++;
    }
  
  double lSizeMat = QueueOfLConstraintInequalities.back()->EndingTime/T;
  MAL_MATRIX_RESIZE(vnlStorePx,
		    NbOfConstraints,
		    //6*N,
		    1+(unsigned int)lSizeMat);
  
  for(unsigned int i=0;i<MAL_MATRIX_NB_ROWS(vnlStorePx);i++)
    {
      for(unsigned int j=0;j<MAL_MATRIX_NB_COLS(vnlStorePx);j++)
	{
	  vnlStorePx(i,j) =0.0;
	}
    }
  MAL_MATRIX_RESIZE(vnlStoreX,
		    2*N,1+(unsigned int)lSizeMat);

  for(unsigned int i=0;i<2*N;i++)
    vnlStoreX(i,0) = 0.0;
  
  MAL_VECTOR_RESIZE(ConstraintNb,
		    1+(unsigned int)lSizeMat);

  // pre computes the matrices needed for the optimization.

  //  OptA = Id + alpha * VPu.Transpose() * VPu + beta * PPu.Transpose() * PPu;
  MAL_MATRIX(lterm1,double);
  lterm1 = MAL_RET_TRANSPOSE(PPu);
  lterm1 = MAL_RET_A_by_B(lterm1, PPu);
  lterm1 = beta * lterm1;

  MAL_MATRIX(lterm2,double);
  lterm2 = MAL_RET_TRANSPOSE(VPu);
  lterm2 = MAL_RET_A_by_B(lterm2, VPu);
  lterm2 = alpha * lterm2;

  MAL_MATRIX_RESIZE(OptA,
		    MAL_MATRIX_NB_ROWS(lterm1),
		    MAL_MATRIX_NB_COLS(lterm1));
  MAL_MATRIX_SET_IDENTITY(OptA);
  OptA = OptA + lterm1 + lterm2;



  if (CriteriaToMaximize==1)
    {
      for(unsigned int i=0;i<2*N;i++)
	for(unsigned int j=0;j<2*N;j++)
	  C[j*2*N+i] = OptA(i,j);

      if (0)
	{
	  ofstream aof;
	  char Buffer[1024];
	  sprintf(Buffer,"C.dat");
	  aof.open(Buffer,ofstream::out);
	  for(unsigned int i=0;i<2*N;i++)
	    {
	      for(unsigned int j=0;j<2*N-1;j++)
		aof << OptA(i,j) << " ";
	      aof << OptA(i,2*N-1);
	      aof << endl;
	    }
	  aof.close(); 
	  
	}
    }
  
  lterm1 = MAL_RET_TRANSPOSE(PPu);
  lterm1 = MAL_RET_A_by_B(lterm1,PPx);
  OptB = MAL_RET_TRANSPOSE(VPu);
  OptB = MAL_RET_A_by_B(OptB,VPx);
  OptB = alpha * OptB;
  OptB = OptB + beta * lterm1;

  if (0)
  {
    ofstream aof;
    char Buffer[1024];
    sprintf(Buffer,"OptB.dat");
    aof.open(Buffer,ofstream::out);
    for(unsigned int i=0;i<MAL_MATRIX_NB_ROWS(OptB);i++)
      {
	for(unsigned int j=0;j<MAL_MATRIX_NB_COLS(OptB)-1;j++)
	  aof << OptB(i,j) << " ";
	aof << OptB(i,MAL_MATRIX_NB_COLS(OptB)-1);
	aof << endl;
      }
    aof.close(); 
    
  }
  
  OptC = MAL_RET_TRANSPOSE(PPu);
  OptC = beta * OptC;
  if (0)
  {
    ofstream aof;
    char Buffer[1024];
    sprintf(Buffer,"OptC.dat");
    aof.open(Buffer,ofstream::out);
    for(unsigned int i=0;i<MAL_MATRIX_NB_ROWS(OptC);i++)
      {
	for(unsigned int j=0;j<MAL_MATRIX_NB_COLS(OptC)-1;j++)
	  aof << OptC(i,j) << " ";
	aof << OptC(i,MAL_MATRIX_NB_COLS(OptC)-1);
	aof << endl;
      }
    aof.close(); 
    
  }
  
  double TotalAmountOfCPUTime=0.0,CurrentCPUTime=0.0;
  struct timeval start,end;
  int li=0; 
  double dinterval = T /  m_SamplingPeriod;
  int interval=(int)dinterval;

  ODEBUG("0.0 " << QueueOfLConstraintInequalities.back()->EndingTime-	N*T << " " 
	  << " T: " << T << " N: " << N << " interval " << interval);
  for(double StartingTime=0.0;
      StartingTime<QueueOfLConstraintInequalities.back()->EndingTime-
	N*T;
      StartingTime+=T,li++)
    {
      gettimeofday(&start,0);
      // Build the related matrices.
      BuildMatricesPxPu(Px,Pu,
			N,T,
			StartingTime,
			QueueOfLConstraintInequalities,
			ComHeight,
			NbOfConstraints,
			xk);
      

      m = NbOfConstraints;
      
      mmax = NbOfConstraints+1;
      lwar = 3*nmax*nmax/2+ 10*nmax  + 2*mmax + 20000;
      mnn = m+n+n;

      // Call to QLD (a linearly constrained quadratic problem solver)

      // Prepare D.
      for(unsigned int i=0;i<N;i++)
	{
	  ZMPRef[i] = ZMPRefPositions[li*interval+i*interval].px;
	  ZMPRef[i+N] = ZMPRefPositions[li*interval+i*interval].py;
	}
      
      if (0)
	{
	  ofstream aof;
	  char Buffer[1024];
	  sprintf(Buffer,"ZMPRef_%f.dat",StartingTime);
	  aof.open(Buffer,ofstream::out);
	  for(unsigned int i=0;i<2*N;i++)
	    {
	      aof << ZMPRef[i] << endl;
	    }
	  aof.close(); 
	}  

      if (CriteriaToMaximize==1)
	{
	  MAL_VECTOR(lterm1v,double);
	  MAL_C_eq_A_by_B(lterm1v,OptC,ZMPRef);
	  MAL_C_eq_A_by_B(OptD,OptB,xk);
	  OptD -= lterm1v;
	  for(unsigned int i=0;i<2*N;i++)
	    D[i] = OptD[i];

	  if (0)
	    {
	      ofstream aof;
	      char Buffer[1024];
	      sprintf(Buffer,"D_%f.dat",StartingTime);
	      aof.open(Buffer,ofstream::out);
	      for(unsigned int i=0;i<2*N;i++)
		{
		  aof << OptD[i] << endl;
		}
	      aof.close(); 
	    }

	}
      else
	{
	  // Default : set D to zero.
	  for(unsigned int i=0;i<2*N;i++)
	    D[i] = 0.0;
	}

      for(unsigned int i=0;i<2*N;i++)
	{
	  XL[i] = -1e8;
	  XU[i] = 1e8;
	}
      memset(X,0,2*N*sizeof(double));

      // Verification
      ConstraintNb[li] = m;
      MAL_MATRIX_RESIZE(vnlPu,m,2*N);
      MAL_MATRIX_RESIZE(vnlPx,m,1);
      
  
      for(int i=0; i<m;i++)
	{
	  vnlPx(i,0) =
	    vnlStorePx(i,li) = Px[i];
	}

      iwar[0]=1;
      ODEBUG("m: " << m);
      ql0001_(&m, &me, &mmax,&n, &nmax,&mnn,
	      C, D, Pu,Px,XL,XU,
	      X,U,&iout, &ifail, &iprint,
	      war, &lwar,
	      iwar, &liwar,&Eps);
      if (ifail!=0)
	{
	  cout << "IFAIL: " << ifail << endl;
	  return -1;
	}


      for(int i=0; i<m;i++)
	for(unsigned int j=0; j<2*N;j++)
	  vnlPu(i,j) = Pu[j*(m+1)+i];

      for(unsigned int i=0; i<2*N;i++)
	{
	  vnlStoreX(i,li) = X[i];
	  vnlX(i,0) = X[i];
	}

      vnlValConstraint = MAL_RET_A_by_B(vnlPu, vnlX)  + vnlPx;
      
      if (0)
      {
	ofstream aof;
	char Buffer[1024];
	sprintf(Buffer,"X_%f.dat",StartingTime);
	aof.open(Buffer,ofstream::out);
	for(unsigned int i=0;i<2*N;i++)
	  {
	    aof << X[i] << endl;
	  }
	aof.close(); 
      }

      if (MAL_MATRIX_NB_COLS(vnlValConstraint)!=1)
	{
	  cout << "Problem during validation of the constraints matrix: " << endl;
	  cout << "   size for the columns different from 1" << endl;
	  return -1;
	}


      for(int i=0;i<m;i++)
	{
	  unsigned int pbOnCurrent=0;
	  if (vnlValConstraint(i,0)<-1e-8)
	    {
	      ODEBUG3("Problem during validation of the constraint: ");
	      ODEBUG3("  constraint " << i << " is not positive");
	      ODEBUG3(vnlValConstraint(i,0));
	      pbOnCurrent = 1;
	    }

	  if (pbOnCurrent)
	    {
	      ODEBUG3("PbonCurrent: " << pbOnCurrent << " " << li
		      << " Contrainte " << i 
		      << " StartingTime :" << StartingTime);
	      if (pbOnCurrent)
		{
		  return -1;
		}
	    }
	    
	}
      ODEBUG("X[0] " << X[0] << " X[N] :" << X[N]);

      // Compute the command multiply 
      Buk[0] = X[0]*m_B(0,0);
      Buk[1] = X[0]*m_B(1,0);
      Buk[2] = X[0]*m_B(2,0);
      
      Buk[3] = X[N]*m_B(3,0);
      Buk[4] = X[N]*m_B(4,0);
      Buk[5] = X[N]*m_B(5,0);


      // Fill the queues with the interpolated CoM values.
      for(int lk=0;lk<interval;lk++)
	{
	  
	  COMPosition aCOMPos;
	  double lkSP;
	  lkSP = (lk+1) * m_SamplingPeriod;

	  aCOMPos.x[0] = 
	    xk[0] + // Position
	    lkSP * xk[1] +  // Speed
	    0.5 * lkSP*lkSP * xk[2] +// Acceleration 
	    lkSP * lkSP * lkSP * X[0] /6.0; // Jerk

	  aCOMPos.x[1] = 
	    xk[1] + // Speed
	    lkSP * xk[2] +  // Acceleration
	    0.5 * lkSP * lkSP * X[0]; // Jerk

	  aCOMPos.x[2] = 
	    xk[2] +  // Acceleration
	    lkSP * X[0]; // Jerk

	  aCOMPos.y[0] = 
	    xk[3] + // Position
	    lkSP * xk[4] +  // Speed
	    0.5 * lkSP*lkSP * xk[5] + // Acceleration 
	    lkSP * lkSP * lkSP * X[N] /6.0; // Jerk

	  aCOMPos.y[1] = 
	    xk[4] + // Speed
	    lkSP * xk[5] +  // Acceleration
	    0.5 * lkSP * lkSP * X[N]; // Jerk

	  aCOMPos.y[2] = 
	    xk[5] +  // Acceleration
	    lkSP * X[N]; // Jerk

	  aCOMPos.yaw = ZMPRefPositions[li*interval+lk].theta;

	  COMPositions.push_back(aCOMPos);

	  // Compute ZMP position and orientation.
	  ZMPPosition aZMPPos;
	  aZMPPos.px = m_C(0,0) * aCOMPos.x[0] +
	    m_C(0,1) * aCOMPos.x[1] + m_C(0,2) * aCOMPos.x[2];
	  
	  aZMPPos.py = m_C(0,0) * aCOMPos.y[0] +
	    m_C(0,1) * aCOMPos.y[1] + m_C(0,2) * aCOMPos.y[2];

	  aZMPPos.theta = ZMPRefPositions[li*interval+lk].theta;
	  aZMPPos.stepType = ZMPRefPositions[li*interval+lk].stepType;

	  // Put it into the stack.
	  NewFinalZMPPositions.push_back(aZMPPos);
	  
	  ODEBUG4(aCOMPos.x[0] << " " << aCOMPos.x[1] << " " << aCOMPos.x[2] << " " <<
		  aCOMPos.y[0] << " " << aCOMPos.y[1] << " " << aCOMPos.y[2] << " " <<
		  aCOMPos.yaw << " " <<
		  aZMPPos.px << " " << aZMPPos.py <<  " " << aZMPPos.theta << " " << 
		  X[0] << " " << X[N] << " " << 
		  lkSP << " " << T , "DebugInterpol.dat");
	}
      // Simulate the dynamical system
      xk = MAL_RET_A_by_B(m_A,xk) + Buk ;
      // Modif. from Dimitar: Initially a mistake regarding the ordering.
      MAL_C_eq_A_by_B(zk,m_C,xk);

      ODEBUG4(xk[0] << " " << xk[1] << " " << xk[2] << " " <<
	      xk[3] << " " << xk[4] << " " << xk[5] << " " <<
	      X[0]  << " " << X[N]  << " " << 
	      zk[0] << " " << zk[1] << " " << 
	      StartingTime
	      ,"DebugPBW.dat");
      ODEBUG6("uk:" << uk,"DebugPBW.dat");
      ODEBUG6("xk:" << xk,"DebugPBW.dat");

      // Compute CPU consumption time.
      gettimeofday(&end,0);
      CurrentCPUTime = end.tv_sec - start.tv_sec + 
	0.000001 * (end.tv_usec - start.tv_usec);
      TotalAmountOfCPUTime += CurrentCPUTime;
      ODEBUG("Current Time : " << StartingTime << " " << 
	     " Virtual time to simulate: " << QueueOfLConstraintInequalities.back()->EndingTime - StartingTime << 
	     "Computation Time " << CurrentCPUTime << " " << TotalAmountOfCPUTime);

    }
  ODEBUG("NewZMPsize: " << NewFinalZMPPositions.size());
  // Current heuristic to complete the ZMP buffer:
  // fill with the last correct value.
  ZMPPosition LastZMPPos = NewFinalZMPPositions.back();

  ODEBUG("N*T/m_SamplingPeriod :" << N*T/m_SamplingPeriod << " " << (int)N*T/m_SamplingPeriod);
  double lLimit = N*T/m_SamplingPeriod;
  int Limit = (int)lLimit;
  
  ODEBUG("Limit: " << Limit);

  // This test is here to compensate for the discrency 
  // between ZMPRef Position and the new one.
  // As the parameters N and T can be different you may have one
  // or two iterations of difference. They are compensate for here.
  // It is assumed that NewFinalZMPPositions.size() < ZMPRefPositions.size()
  if ((Limit + NewFinalZMPPositions.size()) != ZMPRefPositions.size())
    Limit = ZMPRefPositions.size() - NewFinalZMPPositions.size();

  ODEBUG("Limit: " << Limit);
  for (int i=0;i< Limit;i++)
  {
    ZMPPosition aZMPPos;
    aZMPPos.px = LastZMPPos.px;
    aZMPPos.py = LastZMPPos.py;
    aZMPPos.theta = LastZMPPos.theta;
    aZMPPos.stepType = LastZMPPos.stepType;
    NewFinalZMPPositions.push_back(aZMPPos);
  }
  ODEBUG("NewZMPsize: " << NewFinalZMPPositions.size());
  if (0)
    {
      ofstream aof;
      aof.open("StorePx.dat",ofstream::out);
      
      for(unsigned int i=0;i<MAL_MATRIX_NB_ROWS(vnlStorePx);i++)
	{
	  for(unsigned int j=0;j<MAL_MATRIX_NB_COLS(vnlStorePx);j++)
	    {
	      aof << vnlStorePx(i,j) << " ";
	    }
	  aof << endl;
	}
      aof.close();
      
      
      char lBuffer[1024];
      sprintf(lBuffer,"StoreX.dat");
      aof.open(lBuffer,ofstream::out);
      
      for(unsigned int i=0;i<MAL_MATRIX_NB_ROWS(vnlStoreX);i++)
	{
	  for(unsigned int j=0;j<MAL_MATRIX_NB_COLS(vnlStoreX);j++)
	    {
	      aof << vnlStoreX(i,j) << " ";
	    }
	  aof << endl;
	}
      aof.close();
      
      aof.open("Cnb.dat",ofstream::out);
      for(unsigned int i=0;i<MAL_VECTOR_SIZE(ConstraintNb);i++)
	{
	  aof << ConstraintNb[i]<<endl;
	}
      aof.close();
    }
  
  /*  cout << "Size of PX: " << MAL_MATRIX_NB_ROWS(vnlStorePx) << " " 
      << MAL_MATRIX_NB_COLS(vnlStorePx) << " " << endl; */
  delete C;
  delete D;
  delete XL;
  delete XU;
  delete X;
  free(war);
  free(U);
  delete iwar;
  // Clean the queue of Linear Constraint Inequalities.
  //  deque<LinearConstraintInequality_t *>::iterator LCI_it;
  LCI_it = QueueOfLConstraintInequalities.begin();
  while(LCI_it!=QueueOfLConstraintInequalities.end())
    {
      //      cout << *LCI_it << endl; 
      //      cout << (*LCI_it)->StartingTime << " " << (*LCI_it)->EndingTime << endl;
      delete *(LCI_it);
      LCI_it++;
    }
  QueueOfLConstraintInequalities.clear();
  
  return 0;
}


void ZMPQPWithConstraint::GetZMPDiscretization(deque<ZMPPosition> & ZMPPositions,
					       deque<COMPosition> & COMPositions,
					       deque<RelativeFootPosition> &RelativeFootPositions,
					       deque<FootAbsolutePosition> &LeftFootAbsolutePositions,
					       deque<FootAbsolutePosition> &RightFootAbsolutePositions,
					       double Xmax,
					       COMPosition & lStartingCOMPosition,
					       MAL_S3_VECTOR(&,double) lStartingZMPPosition,
					       FootAbsolutePosition & InitLeftFootAbsolutePosition,
					       FootAbsolutePosition & InitRightFootAbsolutePosition)
{
  if (m_ZMPD==0)
    return;
  
  m_ZMPD->GetZMPDiscretization(ZMPPositions,
			       COMPositions,
			       RelativeFootPositions,
			       LeftFootAbsolutePositions,
			       RightFootAbsolutePositions,
			       Xmax,
			       lStartingCOMPosition,
			       lStartingZMPPosition,
			       InitLeftFootAbsolutePosition,
			       InitRightFootAbsolutePosition);

  deque<ZMPPosition> NewZMPPositions;
  ODEBUG("PBW algo set on");
  ODEBUG("Size of COMBuffer: " << m_COMBuffer.size());
  m_COMBuffer.clear();

  BuildZMPTrajectoryFromFootTrajectory(LeftFootAbsolutePositions,
				       RightFootAbsolutePositions,
				       ZMPPositions,
				       NewZMPPositions,
				       m_COMBuffer,
				       m_ConstraintOnX,
				       m_ConstraintOnY,
				       m_QP_T,
				       m_QP_N);
  if (ZMPPositions.size()!=NewZMPPositions.size())
    {
      cout << "Problem here between m_ZMPPositions and new zmp positions" << endl;
      cout << ZMPPositions.size() << " " << NewZMPPositions.size() << endl;
    }
  
  for(unsigned int i=0;i<ZMPPositions.size();i++)
    ZMPPositions[i] = NewZMPPositions[i];
  
}

void ZMPQPWithConstraint::CallMethod(std::string & Method, std::istringstream &strm)
{
  if (Method==":setpbwconstraint")
    {
      string PBWCmd;
      strm >> PBWCmd;
      if (PBWCmd=="XY")
	{
	  strm >> m_ConstraintOnX;
	  strm >> m_ConstraintOnY;
	  cout << "Constraint On X: " << m_ConstraintOnX
	       << " Constraint On Y: " << m_ConstraintOnY << endl;
	}
      else if (PBWCmd=="T")
	{
	  strm >> m_QP_T;
	  cout << "Sampling for the QP " << m_QP_T <<endl;
	}
      else if (PBWCmd=="N")
	{
	  strm >> m_QP_N;
	  cout << "Preview window for the QP " << m_QP_N << endl;
	}
    }

}

void ZMPQPWithConstraint::GetComBuffer(deque<COMPosition> &aCOMBuffer)
{
  for(unsigned int i=0;i<m_COMBuffer.size();i++)
    {
      aCOMBuffer.push_back(m_COMBuffer[i]);
    }
}

int ZMPQPWithConstraint::InitOnLine(deque<ZMPPosition> & FinalZMPPositions,
				    deque<COMPosition> & FinalCOMPositions,
				    deque<FootAbsolutePosition> & FinalLeftFootAbsolutePositions,
				    deque<FootAbsolutePosition> & FinalRightFootAbsolutePositions,
				    FootAbsolutePosition & InitLeftFootAbsolutePosition,
				    FootAbsolutePosition & InitRightFootAbsolutePosition,
				    deque<RelativeFootPosition> &RelativeFootPositions,
				    COMPosition & lStartingCOMPosition,
				    MAL_S3_VECTOR(&,double) lStartingZMPPosition)
{
  cout << "To be implemented" << endl;
  return 0;
}

void ZMPQPWithConstraint::OnLineAddFoot(RelativeFootPosition & NewRelativeFootPosition,
					deque<ZMPPosition> & FinalZMPPositions,	
					deque<COMPosition> & FinalCOMPositions,
					deque<FootAbsolutePosition> &FinalLeftFootAbsolutePositions,
					deque<FootAbsolutePosition> &FinalRightFootAbsolutePositions,
					bool EndSequence)
{
  cout << "To be implemented" << endl;
}

void ZMPQPWithConstraint::OnLine(double time,
				 deque<ZMPPosition> & FinalZMPPositions,				     
				 deque<COMPosition> & FinalCOMPositions,
				 deque<FootAbsolutePosition> &FinalLeftFootAbsolutePositions,
				 deque<FootAbsolutePosition> &FinalRightFootAbsolutePositions)
{
  cout << "To be implemented" << endl;
}

int ZMPQPWithConstraint::OnLineFootChange(double time,
					  FootAbsolutePosition &aFootAbsolutePosition,
					  deque<ZMPPosition> & FinalZMPPositions,			     
					  deque<COMPosition> & CoMPositions,
					  deque<FootAbsolutePosition> &FinalLeftFootAbsolutePositions,
					  deque<FootAbsolutePosition> &FinalRightFootAbsolutePositions,
					  StepStackHandler  *aStepStackHandler)
{
  cout << "To be implemented" << endl;
  return -1;
}

void ZMPQPWithConstraint::EndPhaseOfTheWalking(deque<ZMPPosition> &ZMPPositions,
					       deque<COMPosition> &FinalCOMPositions,
					       deque<FootAbsolutePosition> &LeftFootAbsolutePositions,
					       deque<FootAbsolutePosition> &RightFootAbsolutePositions)
{
  
}

int ZMPQPWithConstraint::ReturnOptimalTimeToRegenerateAStep()
{
  int r = (int)(m_PreviewControlTime/m_SamplingPeriod);
  return 2*r;
}
