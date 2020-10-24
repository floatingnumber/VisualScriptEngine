#ifndef BI_UNARYFUNCTIONNODES_HPP
#define BI_UNARYFUNCTIONNODES_HPP

#include "NE_SingleValues.hpp"
#include "BI_BasicUINode.hpp"
#include "BI_BuiltInFeatures.hpp"

namespace BI
{

class UnaryFunctionNode : public BasicUINode
{
	SERIALIZABLE;

public:
	UnaryFunctionNode ();
	UnaryFunctionNode (const NE::LocString& name, const NUIE::Point& position);
	virtual ~UnaryFunctionNode ();

	virtual void				Initialize () override;
	virtual NE::ValueConstPtr	Calculate (NE::EvaluationEnv& env) const override;
		
	virtual void				RegisterParameters (NUIE::NodeParameterList& parameterList) const override;
	virtual bool				IsForceCalculated () const override;

	virtual NE::Stream::Status	Read (NE::InputStream& inputStream) override;
	virtual NE::Stream::Status	Write (NE::OutputStream& outputStream) const override;

private:
	NE::ValuePtr				DoSingleOperation (const NE::ValueConstPtr& aValue) const;
	virtual double				DoOperation (double a) const = 0;
};

class AbsoluteNode : public UnaryFunctionNode
{
	DYNAMIC_SERIALIZABLE (AbsoluteNode);

public:
	AbsoluteNode ();
	AbsoluteNode (const NE::LocString& name, const NUIE::Point& position);
	virtual ~AbsoluteNode ();

private:
	virtual double DoOperation (double a) const override;
};

class FloorNode : public UnaryFunctionNode
{
	DYNAMIC_SERIALIZABLE (FloorNode);

public:
	FloorNode ();
	FloorNode (const NE::LocString& name, const NUIE::Point& position);
	virtual ~FloorNode ();

private:
	virtual double DoOperation (double a) const override;
};

class CeilNode : public UnaryFunctionNode
{
	DYNAMIC_SERIALIZABLE (CeilNode);

public:
	CeilNode ();
	CeilNode (const NE::LocString& name, const NUIE::Point& position);
	virtual ~CeilNode ();

private:
	virtual double DoOperation (double a) const override;
};

}

#endif
