#pragma once
#include <FECore/FEDataLoadCurve.h>
#include <vector>
#include <string>
using namespace std;

class FEModel;

//=============================================================================
//! This class evaluates the objective function, which is defined as the sum
//! of squares of non-linear functions. 
//            ----
//             \                   2
//  f_obj =    /    [ f_i(a) - y_i]
//            ----
//              i
//
//! It is an abstract base class and derived classes
//! need to implement the nonlinear functions.
class FEObjectiveFunction
{
public:
	// constructor
	FEObjectiveFunction(FEModel* fem);
	virtual ~FEObjectiveFunction();

	// one-time initialization
	virtual bool Init();

	// This is called before each optimization iteration
	// and should be used by derived classes to reset any data
	virtual void Reset();

	// evaluate objective function
	// also returns the function values in f
	virtual double Evaluate(vector<double>& f);

	// evaluate objective function
	double Evaluate();

public: // These functions need to be implemented by derived classes

	// return number of measurements
	virtual int Measurements() = 0;

	// evaluate the function values
	virtual void EvaluateFunctions(vector<double>& f) = 0;

	// get the measurement vector
	virtual void GetMeasurements(vector<double>& y) = 0;

	// return the FE model
	FEModel* GetFEM() { return m_fem; }

private:
	FEModel*	m_fem;
};

//=============================================================================
// Objective function for fitting time-value data
// In this case the nonlinear functions are definde as:
// f_i(a) = F(t_i; a)
// where t_i is the time for time step i, and F is the function that will be fitted to the measurement vector.
class FEDataFitObjective : public FEObjectiveFunction
{
public:
	FEDataFitObjective(FEModel* fem);

	// one-time initialization
	bool Init();

	// This is called before each optimization iteration
	// and should be used by derived classes to reset any data
	void Reset();

public:
	// get the reaction load curve
	FEDataLoadCurve& ReactionLoad() { return m_rf; }

	// get the load curve providing the measurement vector
	FEDataLoadCurve& GetDataCurve() { return m_lc; }

public:
	// return number of measurements
	int Measurements();

	// evaluate the function values
	void EvaluateFunctions(vector<double>& f);

	// get the measurement vector
	void GetMeasurements(vector<double>& y);

public:
	string	m_name;			//!< name of parameter that generates the function data
	double*	m_pd;			//!< pointer to variable data

	FEDataLoadCurve		m_rf;		//!< reaction force data
	FEDataLoadCurve		m_lc;
};

//=============================================================================
// Objective function for minimization of model parameters
class FEMinimizeObjective : public FEObjectiveFunction
{
	class Function
	{
	public:
		string		name;
		double*		var;

		double	y0;		// target value (i.e. "measurment")

	public:
		Function() : var(0), y0(0.0) {}
		Function(const Function& f) { name = f.name; var = f.var; y0 = f.y0; }
		void operator = (const Function& f) { name = f.name; var = f.var; y0 = f.y0; }
	};

public:
	FEMinimizeObjective(FEModel* fem);

	// one-time initialization
	bool Init();

	bool AddFunction(const char* szname, double targetValue);

public:
	// return number of measurements
	int Measurements();

	// evaluate the function values
	void EvaluateFunctions(vector<double>& f);

	// get the measurement vector
	void GetMeasurements(vector<double>& y);

private:
	std::vector<Function>	m_Func;
};