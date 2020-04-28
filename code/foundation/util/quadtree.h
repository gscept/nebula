#pragma once
//------------------------------------------------------------------------------
/**
    @class QuadTree
    @ingroup Util

    A simple quad tree. QuadTree elements are template nodes and
    are inserted and removed from a quadtree by bounding box.

    (C) 2007 RadonLabs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/fixedarray.h"
#include "math/bbox.h"
#include "math/vec4.h"

namespace Util
{
//------------------------------------------------------------------------------
template<class TYPE> class QuadTree
{
public:
    class Node;    

    /// constructor
    QuadTree();
    /// destructor
    ~QuadTree();
    /// initialize quad tree
    void Setup(const Math::bbox& box, uchar depth);
    /// get the top level bounding box
    const Math::bbox& GetBoundingBox() const;
    /// get the tree depth
    uchar GetDepth() const;
    /// compute number of nodes in a level, including its children
    SizeT GetNumNodes(uchar level) const;
    /// compute linear chunk index from level, col and row
    IndexT GetNodeIndex(uchar level, ushort col, ushort row) const;
    /// get overall number of nodes in the tree
    SizeT GetNumNodesInTree() const;
    /// get pointer to node by index
    const Node& GetNodeByIndex(IndexT i) const;
    /// read/write access to node
    Node& NodeByIndex(IndexT i);
    /// recursively find the smallest child node which contains the bounding box
    Node* FindContainmentNode(const Math::bbox& box);

    /// node in quad tree
    class Node
    {
    public:
        /// constructor
        Node();
        /// destructor
        ~Node();
        /// recursively initialize the node
        void Setup(QuadTree<TYPE>* tree, uchar _level, ushort _col, ushort _row);
        /// get the node's level
        char Level() const;
        /// get the node's column
        ushort Column() const;
        /// get the node's row
        ushort Row() const;
        /// compute the node's bounding box
        const Math::bbox& GetBoundingBox() const;
        /// recursively find the smallest child node which contains the bounding box
        Node* FindContainmentNode(const Math::bbox& box);
        /// set data element associated with node
        void SetElement(const TYPE& elm);
        /// get data element
        const TYPE& GetElement() const;
        /// get child at index
        Node* GetChildAt(IndexT i);

    private:
        friend class QuadTree;

        Node* children[4];
        char level;
        ushort col;
        ushort row;
        Math::bbox box;
        TYPE element;
    };

private:
    friend class Node;

    uchar treeDepth;
    Math::bbox boundingBox;                  // global bounding box
    Math::vector baseNodeSize;               // base chunk bounding box
    Util::FixedArray<Node> nodeArray;
};

//------------------------------------------------------------------------------
/**
    QuadTree constructor.
*/
template<class TYPE>
QuadTree<TYPE>::QuadTree() :
    treeDepth(0),
    baseNodeSize(0.0f, 0.0f, 0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    QuadTree destructor.
*/
template<class TYPE>
QuadTree<TYPE>::~QuadTree()
{
    // empty
}



//------------------------------------------------------------------------------
/**
    Initialize the quad tree.
*/
template<class TYPE> void
QuadTree<TYPE>::Setup(const Math::bbox& box, uchar depth)
{
    #if NEBULA_BOUNDSCHECKS    
    n_assert(depth > 0);
    #endif

    this->treeDepth = depth;
    this->boundingBox = box;

    int baseDimension = 1 << (this->treeDepth - 1);
    this->baseNodeSize.set(this->boundingBox.size().x / baseDimension,
                           this->boundingBox.size().y,                                                                          
                           this->boundingBox.size().z / baseDimension);

    SizeT numNodes = this->GetNumNodes(this->treeDepth);
    this->nodeArray.SetSize(numNodes);
    this->nodeArray[0].Setup(this, 0, 0, 0);

    // make sure all nodes have been initialized
    #if NEBULA_BOUNDSCHECKS
    int i;
    int num = this->nodeArray.Size();
    for (i = 0; i < num; i++)
    {
        n_assert(this->nodeArray[i].Level() >= 0);
    }
    #endif
}

//------------------------------------------------------------------------------
/**
    Returns depth of quad tree.
*/
template<class TYPE> uchar
QuadTree<TYPE>::GetDepth() const
{
    return this->treeDepth;
}

//------------------------------------------------------------------------------
/**
    Returns top level bounding box of quad tree.
*/
template<class TYPE> const Math::bbox&
QuadTree<TYPE>::GetBoundingBox() const
{
    return this->boundingBox;
}

//------------------------------------------------------------------------------
/**
    Computes number of nodes in a level, including its child nodes.
*/
template<class TYPE> SizeT
QuadTree<TYPE>::GetNumNodes(uchar level) const
{
	//FIXME WTF
    return 0x55555555 & ((1 << level * 2) - 1);
}

//------------------------------------------------------------------------------
/**
    Returns the overall number of nodes in the tree for linear access.
*/
template<class TYPE> SizeT
QuadTree<TYPE>::GetNumNodesInTree() const
{
    return this->nodeArray.Size();
}

//------------------------------------------------------------------------------
/**
    Computes a linear chunk index for a chunk address consisting of 
    level, col and row.
*/
template<class TYPE> IndexT
QuadTree<TYPE>::GetNodeIndex(uchar level, ushort col, ushort row) const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert((col >= 0) && (col < (1 << level)));
    n_assert((row >= 0) && (row < (1 << level)));
    #endif
    return this->GetNumNodes(level) + (row << level) + col;
}

//------------------------------------------------------------------------------
/**
    Find the biggest quad tree which completely contains the provided
    bounding box.
*/
template<class TYPE> typename QuadTree<TYPE>::Node*
QuadTree<TYPE>::FindContainmentNode(const Math::bbox& checkBox)
{
    return this->nodeArray[0].FindContainmentNode(checkBox);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
QuadTree<TYPE>::Node::Node() :
    level(-1),
    col(0),
    row(0)
{
    int i;
    for (i = 0; i < 4; i++)
    {
        this->children[i] = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
QuadTree<TYPE>::Node::~Node()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Recursively initialize a quad tree node.
*/
template<class TYPE> void
QuadTree<TYPE>::Node::Setup(QuadTree* tree, uchar _level, ushort _col, ushort _row)
{
    #if NEBULA_BOUNDSCHECKS
    n_assert(tree);
    n_assert(this->level == -1);
    n_assert(_col < (1 << _level));
    n_assert(_row < (1 << _level));
    #endif

    // store address
    this->level = _level;
    this->col   = _col;
    this->row   = _row;

    // update my bounding box
    float levelFactor = float(1 << (tree->treeDepth - 1 - this->level));
    static Math::vector center;
    static Math::vector extent;
    const Math::vector& baseSize = tree->baseNodeSize;
    const Math::bbox& treeBox = tree->boundingBox;
    Math::vector treeSize = treeBox.size();
    Math::vector treeCenter = treeBox.center();

    center.set(treeCenter.x + (((this->col + 0.5f) * levelFactor * baseSize.x) - (treeSize.x * 0.5f)),
               treeCenter.y,
               treeCenter.z + (((this->row + 0.5f) * levelFactor * baseSize.z) - (treeSize.z * 0.5f)));

    extent.set(levelFactor * baseSize.x * 0.5f,
               treeBox.extents().y,
               levelFactor * baseSize.z * 0.5f );

    this->box.set(center, extent);

    // recurse into children
    uchar childLevel = this->level + 1;
    if (childLevel < tree->treeDepth)
    {
        ushort i;
        for (i = 0; i < 4; i++)
        {
            ushort childCol = 2 * this->col + (i & 1);
            ushort childRow = 2 * this->row + ((i & 2) >> 1);
            IndexT childIndex = tree->GetNodeIndex(childLevel, childCol, childRow);
            this->children[i] = &(tree->nodeArray[childIndex]);
            this->children[i]->Setup(tree, childLevel, childCol, childRow);
        }
    }
}

//------------------------------------------------------------------------------
/**
    This finds the smallest child node which completely contains the
    given bounding box. Calls itself recursively.
*/
template<class TYPE> typename QuadTree<TYPE>::Node*
QuadTree<TYPE>::Node::FindContainmentNode(const Math::bbox& checkBox)
{
    if (this->box.contains(checkBox))
    {
        // recurse into children
        if (this->children[0] != 0)
        {
            int i;
            for (i = 0; i < 4; i++)
            {
                Node* containNode = this->children[i]->FindContainmentNode(checkBox);
                if (containNode)
                {
                    return containNode;
                }
            }
        }

        // not contained in children, but still contained in this
        return this;
    }
    else
    {
        // not contained in this, break recursion
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> const typename QuadTree<TYPE>::Node&
QuadTree<TYPE>::GetNodeByIndex(IndexT index) const
{
    return this->nodeArray[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> typename QuadTree<TYPE>::Node&
QuadTree<TYPE>::NodeByIndex(IndexT index)
{
    return this->nodeArray[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> const Math::bbox&
QuadTree<TYPE>::Node::GetBoundingBox() const
{
    return this->box;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> char
QuadTree<TYPE>::Node::Level() const
{
    return this->level;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> ushort
QuadTree<TYPE>::Node::Column() const
{
    return this->col;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> ushort
QuadTree<TYPE>::Node::Row() const
{
    return this->row;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
QuadTree<TYPE>::Node::SetElement(const TYPE& elm)
{
    this->element = elm;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> const TYPE&
QuadTree<TYPE>::Node::GetElement() const
{
    return this->element;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> typename QuadTree<TYPE>::Node*
QuadTree<TYPE>::Node::GetChildAt(IndexT i)
{
    return this->children[i];
}
} // namespace Util
//------------------------------------------------------------------------------
