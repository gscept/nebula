//------------------------------------------------------------------------------
//  physics/materialtable.cc
//  (C) 2003 RadonLabs GmbH
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/materialtable.h"
#include "io/excelxmlreader.h"
#include "io/filestream.h"
#include "io/ioserver.h"

namespace Physics
{
using namespace Util;

Util::HashTable<Util::StringAtom, MaterialType> MaterialTable::materialsHash;
Util::FixedArray<struct MaterialTable::Material> MaterialTable::materials;
Util::FixedTable<struct MaterialTable::Interaction> MaterialTable::interactions;

Util::StringAtom MaterialTable::invalidTypeString = "InvalidMaterial";

//------------------------------------------------------------------------------
/**
*/
void
MaterialTable::Setup()
{
	Util::String path = "root:data/tables/physicsmaterials.xml";
	if (IO::IoServer::Instance()->FileExists(path))
	{
		Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(path);
		stream->SetAccessMode(IO::Stream::ReadAccess);
		Util::Array<Material> mats;
		Util::Array<String> inters;
		Ptr<IO::XmlReader> reader = IO::XmlReader::Create();
		{
			reader->SetStream(stream);
			if (reader->Open())
			{
				if (reader->HasNode("/PhysicsMaterials"))
				{
					reader->SetToFirstChild();
					{
						do
						{
							Material mat;
							mat.name = reader->GetString("Id");
							mat.restitution = reader->GetFloat("Restitution");
							mat.friction = reader->GetFloat("Friction");
							mats.Append(mat);
							inters.Append(reader->GetString("CollisionEvents"));
						} while (reader->SetToNextChild());
					}
				}
				reader->Close();
			}
		}
		if (!mats.IsEmpty())
		{
			int matCount = mats.Size();
			materials.SetSize(matCount);
			interactions.SetSize(matCount, matCount);
			materialsHash.Clear();
			for (int i = 0; i < mats.Size(); i++)
			{
				AddMaterial(i, mats[i].name, mats[i].friction, mats[i].restitution);
			}
			for (int i = 0; i < inters.Size(); i++)
			{
				Util::Dictionary<Util::String, Util::String> events = Util::String::ParseKeyValuePairs(inters[i]);
				for (int j = 0; j < events.Size(); j++)
				{
					Util::String otherMat = events.KeyAtIndex(j);
					Util::String ev = events.ValueAtIndex(j);
					n_assert(materialsHash.Contains(otherMat));
					AddInteraction(i, materialsHash[otherMat], ev, false);
				}
			}

		}
	}
	
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialTable::AddMaterial(MaterialType t, const Util::StringAtom& name, float friction, float restitution)
{
	n_assert(t >= 0 && t < materials.Size());
	materials[t].friction = friction;
	materials[t].restitution = restitution;
	materials[t].name = name;
	materialsHash.Add(name, t);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialTable::AddInteraction(MaterialType t0, MaterialType t1, const Util::StringAtom& event, bool symmetric)
{
	Interaction inter;
	inter.collEvent = event;	
	interactions.Set(t0, t1, inter);
	if (symmetric)
	{
		interactions.Set(t1, t0, inter);
	}
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom&
MaterialTable::MaterialTypeToString(MaterialType t)
{
    if (-1 == t)
    {
        return invalidTypeString;
    }
    else
    {
        n_assert(t >= 0 && (IndexT)t < materials.Size());
        return materials[t].name;
    }
}

//------------------------------------------------------------------------------
/**
*/
MaterialType 
MaterialTable::StringToMaterialType(const Util::StringAtom& str)
{
	if (materialsHash.Contains(str))
	{
		return materialsHash[str];
	}
	else
	{
		return InvalidMaterial;
	}	
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialTable::SetFriction(MaterialType t0, float friction)
{
    n_assert(t0 != -1 && t0 < materials.Size());    
    materials[t0].friction = friction;
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialTable::SetRestitution(MaterialType t0, float restitution)
{
    n_assert(t0 != -1 && t0 < materials.Size());    
	materials[t0].restitution = restitution;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialTable::SaveMaterialTable(const IO::URI & file)
{

}

//------------------------------------------------------------------------------
/**
*/
Util::Array<MaterialTable::Material>
MaterialTable::GetMaterialTable()
{
    Util::Array<Material> ret;
    for (int i = 0; i < materials.Size(); i++)
    {
        ret.Append(materials[i]);
    }
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Util::Dictionary<Util::String, Util::Dictionary<Util::String, Util::String>>
MaterialTable::GetInteractionTable()
{
    Util::Dictionary<Util::String, Util::Dictionary<Util::String, Util::String>> ret;
    for (int i = 0; i < materials.Size(); i++)
    {
        Util::Dictionary<Util::String, Util::String> inters;
        for (int j = i; j < materials.Size(); j++)
        {
            inters.Add(materials[j].name.AsString(), GetCollisionEvent(i, j).AsString());
        }
        ret.Add(materials[i].name.AsString(), inters);
    }
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Util::StringAtom>
MaterialTable::GetMaterials()
{
    Util::Array<Util::StringAtom> mats = materialsHash.KeysAsArray();
    mats.Append(MaterialTable::invalidTypeString);
    return mats;
}

} // namespace Physics