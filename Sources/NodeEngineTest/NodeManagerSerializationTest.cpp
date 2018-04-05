#include "SimpleTest.hpp"
#include "NE_NodeManager.hpp"
#include "NE_Node.hpp"
#include "NE_MemoryStream.hpp"
#include "NE_SingleValues.hpp"

#include <memory>

using namespace NE;

namespace NodeManagerSerializationTest
{

class TestNode : public Node
{
	DYNAMIC_SERIALIZABLE (TestNode);

public:
	TestNode () :
		TestNode (0)
	{
	
	}

	TestNode (int val) :
		Node (),
		val (val)
	{
	
	}

	int GetVal () const
	{
		return val;
	}

	virtual void RegisterSlots () override
	{
		RegisterInputSlot (InputSlotPtr (new InputSlot (SlotId ("a"), ValuePtr (new IntValue (0)), OutputSlotConnectionMode::Single)));
		RegisterInputSlot (InputSlotPtr (new InputSlot (SlotId ("b"), ValuePtr (new IntValue (0)), OutputSlotConnectionMode::Single)));
		RegisterOutputSlot (OutputSlotPtr (new OutputSlot (SlotId ("c"))));
	}

	virtual ValuePtr Calculate (NE::EvaluationEnv& env) const override
	{
		ValuePtr firstResult = EvaluateSingleInputSlot (SlotId ("a"), env);
		ValuePtr secondResult = EvaluateSingleInputSlot (SlotId ("b"), env);
		return ValuePtr (new IntValue (val + IntValue::Get (firstResult) + IntValue::Get (secondResult)));
	}

	virtual Stream::Status Read (InputStream& inputStream) override
	{
		ObjectHeader header (inputStream);
		Node::Read (inputStream);
		inputStream.Read (val);
		return inputStream.GetStatus ();
	}

	virtual Stream::Status Write (OutputStream& outputStream) const override
	{
		ObjectHeader header (outputStream, serializationInfo);
		Node::Write (outputStream);
		outputStream.Write (val);
		return outputStream.GetStatus ();
	}

private:
	int val;
};

DynamicSerializationInfo TestNode::serializationInfo (ObjectId ("{9E0304A4-3B92-4EFA-9846-F0372A633038}"), ObjectVersion (1), TestNode::CreateSerializableInstance);

static bool ReadWrite (const NodeManager& source, NodeManager& target)
{
	MemoryOutputStream outputStream;
	if (source.Write (outputStream) != Stream::Status::NoError) {
		return false;
	}
	MemoryInputStream inputStream (outputStream.GetBuffer ());
	if (target.Read (inputStream) != Stream::Status::NoError) {
		return false;
	}
	return true;
}

TEST (EmptyNodeManagerSerializationTest)
{
	NodeManager source;
	NodeManager target;
	ASSERT (ReadWrite (source, target));
	ASSERT (source.GetNodeCount () == target.GetNodeCount ());
}

TEST (OneNodeSerializazionTest)
{
	NodeManager source;
	std::shared_ptr<TestNode> sourceNode (new TestNode (42));
	source.AddNode (sourceNode);

	NodeManager target;
	ASSERT (ReadWrite (source, target));
	ASSERT (source.GetNodeCount () == target.GetNodeCount ());
	ASSERT (target.ContainsNode (sourceNode->GetId ()));
	std::shared_ptr<TestNode> targetNode = std::dynamic_pointer_cast<TestNode> (target.GetNode (sourceNode->GetId ()));
	ASSERT (targetNode != nullptr);
	ASSERT (sourceNode != targetNode);
	ASSERT (sourceNode->GetVal () == targetNode->GetVal ());
}

TEST (ConnectionSerializationTest)
{
	NodeManager source;
	std::shared_ptr<TestNode> sourceNode1 (new TestNode (1));
	std::shared_ptr<TestNode> sourceNode2 (new TestNode (2));
	std::shared_ptr<TestNode> sourceNode3 (new TestNode (3));
	source.AddNode (sourceNode1);
	source.AddNode (sourceNode2);
	source.AddNode (sourceNode3);
	source.ConnectOutputSlotToInputSlot (sourceNode1->GetOutputSlot (SlotId ("c")), sourceNode3->GetInputSlot (SlotId ("a")));
	source.ConnectOutputSlotToInputSlot (sourceNode2->GetOutputSlot (SlotId ("c")), sourceNode3->GetInputSlot (SlotId ("b")));
	ValuePtr sourceValue = sourceNode3->Evaluate (EmptyEvaluationEnv);
	ASSERT (IntValue::Get (sourceValue) == 6);

	NodeManager target;
	ASSERT (ReadWrite (source, target));
	ASSERT (source.GetNodeCount () == target.GetNodeCount ());
	std::shared_ptr<TestNode> targetNode3 = std::dynamic_pointer_cast<TestNode> (target.GetNode (sourceNode3->GetId ()));
	ValuePtr targetValue = targetNode3->Evaluate (EmptyEvaluationEnv);
	ASSERT (IntValue::Get (targetValue) == 6);
}

TEST (IdGeneratorConsistencyTest)
{
	NodeManager source;
	source.AddNode (NodePtr (new TestNode (42)));

	NodeManager target;
	ASSERT (ReadWrite (source, target));
	target.AddNode (NodePtr (new TestNode (43)));
}

}
