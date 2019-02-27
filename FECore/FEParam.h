#pragma once
#include "vec3d.h"
#include "mat3d.h"
#include <assert.h>
#include <vector>
#include "fecore_api.h"
#include "ParamString.h"

//-----------------------------------------------------------------------------
class FEParamValidator;
class DumpStream;
class FEParamContainer;

//-----------------------------------------------------------------------------
// Different supported parameter types
enum FEParamType {
	FE_PARAM_INVALID,
	FE_PARAM_INT,
	FE_PARAM_BOOL,
	FE_PARAM_DOUBLE,
	FE_PARAM_VEC2D,
	FE_PARAM_VEC3D,
	FE_PARAM_MAT3D,
	FE_PARAM_MAT3DS,
	FE_PARAM_IMAGE_3D,
	FE_PARAM_STRING,
	FE_PARAM_DATA_ARRAY,
	FE_PARAM_TENS3DRS,
	FE_PARAM_STD_STRING,
	FE_PARAM_STD_VECTOR_DOUBLE,
	FE_PARAM_STD_VECTOR_VEC2D,
	FE_PARAM_STD_VECTOR_STRING,
	FE_PARAM_DOUBLE_MAPPED,
	FE_PARAM_VEC3D_MAPPED,
	FE_PARAM_MAT3D_MAPPED,
	FE_PARAM_MATERIALPOINT
};

//-----------------------------------------------------------------------------
// Parameter flags
enum FEParamFlag {
	FE_PARAM_ATTRIBUTE = 0x01		// parameter will be read as attribute
};

class FEParam;

//-----------------------------------------------------------------------------
// class describing the value of parameter
class FEParamValue
{
private:
	void*			m_pv;		// pointer to variable data
	FEParamType		m_itype;	// type of variable (this is not the type of the param!)
	FEParam*		m_param;	// the parameter (can be null if it is not a parameter)

public:

	FEParamValue()
	{
		m_pv = 0;
		m_itype = FE_PARAM_INVALID;
		m_param = 0;
	}

	explicit FEParamValue(FEParam* p, void* v, FEParamType itype)
	{
		m_pv = v;
		m_itype = itype;
		m_param = p;
	}

	FEParamValue(double& v) : FEParamValue(0, &v, FE_PARAM_DOUBLE) {}
	FEParamValue(vec2d&  v) : FEParamValue(0, &v, FE_PARAM_VEC2D) {}
	FEParamValue(vec3d&  v) : FEParamValue(0, &v, FE_PARAM_VEC3D) {}
	FEParamValue(mat3ds& v) : FEParamValue(0, &v, FE_PARAM_MAT3DS) {}

	bool isValid() const { return (m_pv != 0); }

	FEParamType type() const { return m_itype; }

	void* data_ptr() const { return m_pv; }

	FEParam* param() { return m_param; }

	template <typename T> T& value() { return *((T*)m_pv); }
	template <typename T> const T& value() const { return *((T*)m_pv); }

	FECORE_API FEParamValue component(int n);
};

//-----------------------------------------------------------------------------
//! This class describes a user-defined parameter
class FECORE_API FEParam
{
private:
	void*			m_pv;		// pointer to variable data
	int				m_dim;		// dimension (in case data is array)
	FEParamType		m_type;		// type of variable
	unsigned int	m_flag;		// parameter flags
	unsigned int	m_id;

	const char*	m_szname;	// name of the parameter
	const char*	m_szenum;	// enumerate values for ints

	// parameter validator
	FEParamValidator*	m_pvalid;

	FEParamContainer* m_parent;	// parent object of parameter

public:
	// constructor
	FEParam(void* pdata, FEParamType itype, int ndim, const char* szname);
	FEParam(const FEParam& p);
	FEParam& operator = (const FEParam& p);

	// set the parameter's validator
	void SetValidator(FEParamValidator* pvalid);

	// see if the parameter's value is valid
	bool is_valid() const;

	// return the name of the parameter
	const char* name() const;

	// return the enum values
	const char* enums() const;

	// return the id
	unsigned int id() const;

	// set the enum values (\0 separated. Make sure the end of the string has two \0's)
	void SetEnums(const char* sz);

	// parameter dimension
	int dim() const;

	// parameter type
	FEParamType type() const;

	// data pointer
	void* data_ptr() const;

	// get the param value
	FEParamValue paramValue(int i = -1);

	// Copy the state of one parameter to this parameter.
	// This requires that the parameters are compatible (i.e. same type, etc.)
	bool CopyState(const FEParam& p);

	void setParent(FEParamContainer* pc);
	FEParamContainer* parent();

	void SetFlags(unsigned int flags);
	unsigned int GetFlags() const;

public:
	void Serialize(DumpStream& ar);

public:
	//! retrieves the value for a non-array item
	template <class T> T& value() { return *((T*) data_ptr()); }

	//! retrieves the value for a non-array item
	template <class T> const T& value() const { return *((T*) data_ptr()); }

	//! retrieves the value for an array item
	template <class T> T* pvalue() { return (T*) data_ptr(); }

	//! retrieves the value for an array item
	template <class T> T& value(int i) { return ((T*)data_ptr())[i]; }
	template <class T> T value(int i) const { return ((T*) data_ptr())[i]; }

	//! retrieves pointer to element in array
	template <class T> T* pvalue(int n);

	//! override the template for char pointers
	char* cvalue();
};

//-----------------------------------------------------------------------------
//! Retrieves a pointer to element in array
template<class T> inline T* FEParam::pvalue(int n)
{
	assert((n >= 0) && (n < m_dim));
	return &(pvalue<T>()[n]);
}

//-----------------------------------------------------------------------------
FECORE_API FEParamValue GetParameterComponent(const ParamString& paramName, FEParam* param);
