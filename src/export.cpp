#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <exception>
#include <functional>
#include <unordered_map>
#include <memory>

using namespace std;

class BaseObject
{
public:
	virtual void Print(ostream& out) = 0;
	virtual string AssignedTo() const
	{
		if (assignedName.find(" ") != -1)
			throw runtime_error("Not supporting names with spaces atm");

		return assignedName;
	}


	virtual void AssignTo(const string& str)
	{
		assignedName = str;
	}

	virtual ~BaseObject() = default;

protected:


	string assignedName;

};


class TypedArrayObject
	: public BaseObject
{
public:
	enum class Type
	{
		Float32 = 1,
		Uint8,
		Uint16,
		Uint32
	};

	Type DataType;
	vector<char> DataBuffer;

	virtual unsigned DataSize() const
	{
		
		switch (DataType)
		{
		case Type::Float32:
		case Type::Uint32:
			return 4;
		case Type::Uint16:
			return 2;
		case Type::Uint8:
			return 1;
		default:
			throw runtime_error("Bad Type");
		}
	}

	virtual const char* ConstructorString() const
	{
		switch (DataType)
		{
		case Type::Float32:
			return "new Float32Array";
		case Type::Uint32:
			return "new Uint32Array";
		case Type::Uint16:
			return "new Uint16Array";
		case Type::Uint8:
			return "new Uint8Array";
		default:
			throw runtime_error("Bad Type");
		}
	}

	virtual unsigned Length() const
	{
		return DataBuffer.size() / DataSize();
	}

	virtual void WriteData(ostream& out)
	{
		if (Length() == 0)
			return;

		switch (DataType)
		{
		case Type::Float32:
			copy_n(Data<float>(), Length() - 1, ostream_iterator<float>(out, ","));
			out << *(Data<float>() + Length() - 1);
			break;
		case Type::Uint32:
			copy_n(Data<uint32_t>(), Length() - 1, ostream_iterator<unsigned>(out, ","));
			out << (unsigned)*(Data<uint32_t>() + Length() - 1);
			break;
		case Type::Uint16:
			copy_n(Data<uint16_t>(), Length() - 1, ostream_iterator<unsigned>(out, ","));
			out << (unsigned)*(Data<uint16_t>() + Length() - 1);
			break;
		case Type::Uint8:
			copy_n(Data<uint8_t>(), Length() - 1, ostream_iterator<unsigned>(out, ","));
			out << (unsigned)*(Data<uint8_t>() + Length() - 1);
			break;
		}
	}

	template<typename T>
	T* Data() const
	{
		return (T*)DataBuffer.data();
	}

	TypedArrayObject(Type type)
		: DataType(type)
	{

	}

	TypedArrayObject(Type type, unsigned size)
		: DataType(type), DataBuffer(DataSize() * size)
	{

	}

	TypedArrayObject(const vector<uint8_t>& v)
		: DataType(Type::Uint8), DataBuffer((char*)v.data(), (char*)(v.data() + v.size()))
	{

	}

	TypedArrayObject(const vector<uint16_t>& v)
		: DataType(Type::Uint16), DataBuffer((char*)v.data(), (char*)(v.data() + v.size()))
	{

	}

	TypedArrayObject(const vector<uint32_t>& v)
		: DataType(Type::Uint32), DataBuffer((char*)v.data(), (char*)(v.data() + v.size()))
	{

	}

	TypedArrayObject(const vector<float>& v)
		: DataType(Type::Float32), DataBuffer((char*)v.data(), (char*)(v.data() + v.size()))
	{

	}

	virtual void Print(ostream& out) override
	{
		out << ConstructorString() << "([";
		WriteData(out);
		out << "])";
	}
protected:

};


//All of this could be magiced up via templates. Buuuuuuuut It's 4:30 AM.
class StringObject
	: public BaseObject
{
public:
	string Value;

	virtual void Print(ostream& out) override
	{
		out << "\"" << Value << "\"";
	}

	StringObject(const string& str)
		: Value(str)
	{

	}
};

class IntegerObject
	: public BaseObject
{
public:
	int Value;

	virtual void Print(ostream& out) override
	{
		out << Value;
	}

	IntegerObject(const int i)
		: Value(i)
	{

	}
};

class FloatObject
	: public BaseObject
{
public:
	float Value;

	virtual void Print(ostream& out) override
	{
		out << Value;
	}

	FloatObject(const float i)
		: Value(i)
	{

	}
};



class Object
	: public BaseObject	
{
public:

	void Set(const string& name, unique_ptr<BaseObject>&& object)
	{
		auto f = objects.find(name);
		if (f == objects.end())
		{
			object->AssignTo(name);
			objects.emplace_hint(f, name, move(object));
		}
		else
		{
			f->second->AssignTo("");
			object->AssignTo(name);
			objects[name] = move(object);
			cout << "Replacement of existing object, was this supposed to happen?" << endl;
		}
	}

	void Set(const string& name, const string& value)
	{
		Set(name, unique_ptr<BaseObject>(new StringObject(value)));
	}

	void Set(const string& name, const vector<uint8_t>& value)
	{
		Set(name, unique_ptr<BaseObject>(new TypedArrayObject(value)));
	}

	void Set(const string& name, const vector<uint16_t>& value)
	{
		Set(name, unique_ptr<BaseObject>(new TypedArrayObject(value)));
	}

	void Set(const string& name, const vector<uint32_t>& value)
	{
		Set(name, unique_ptr<BaseObject>(new TypedArrayObject(value)));
	}

	void Set(const string& name, const vector<float>& value)
	{
		Set(name, unique_ptr<BaseObject>(new TypedArrayObject(value)));
	}

	void Set(const string& name, const int i)
	{
		Set(name, unique_ptr<BaseObject>(new IntegerObject(i)));
	}

	void Set(const string& name, const float f)
	{
		Set(name, unique_ptr<BaseObject>(new FloatObject(f)));
	}

	BaseObject* Get(const string& name)
	{
		auto f = objects.find(name);
		if (f == objects.end())
			return nullptr;
		
		return f->second.get();
	}


	virtual void Print(ostream& out) override
	{
		vector<BaseObject*> sortedObjects(0);
		sortedObjects.reserve(objects.size());
		for (auto&& obj : objects)
		{
			sortedObjects.emplace_back(obj.second.get());
		}

		sort(sortedObjects.begin(), sortedObjects.end(), [](const BaseObject* a, const BaseObject* b){
			return a->AssignedTo() > b->AssignedTo();
		});

		out << " { ";

		for (int i = 0; i < sortedObjects.size() - 1; i++)
		{
			out << sortedObjects[i]->AssignedTo() << " : ";
			sortedObjects[i]->Print(out);
			out << ", ";
		}

		if (sortedObjects.size() > 0)
		{
			out << sortedObjects[sortedObjects.size() - 1]->AssignedTo() << " : ";
			sortedObjects[sortedObjects.size() - 1]->Print(out);
		}

		out << " } ";
	}

protected:
	unordered_map<string, unique_ptr<BaseObject>> objects;
};


unique_ptr<TypedArrayObject> LoadVector(const aiVector3D& v)
{
	return nullptr;
}

unique_ptr<TypedArrayObject> LoadColor(const aiColor3D& v)
{
	unique_ptr<TypedArrayObject> obj(new TypedArrayObject(TypedArrayObject::Type::Float32, 3));
	copy_n(&v.r, 3, obj->Data<float>());
	return move(obj);
}

unique_ptr<TypedArrayObject> LoadMatrix(const aiMatrix4x4& mat)
{
	unique_ptr<TypedArrayObject> obj(new TypedArrayObject(TypedArrayObject::Type::Float32, 16));
	copy_n(&mat.a1, 16, obj->Data<float>());
	return move(obj);
}

unique_ptr<Object> LoadMaterial(const aiMaterial* mat)
{
	unique_ptr<Object> matObj(new Object());
	aiString tempString, diffuseTexture, normalTexture;
	aiColor3D diffuseColor = { 1, 1, 1 },
		specularColor = { 1, 1, 1 },
		ambientColor = { 0.1, 0.1, 0.1 };

	mat->Get(AI_MATKEY_NAME, tempString);
	mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
	mat->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
	mat->Get(AI_MATKEY_COLOR_AMBIENT, ambientColor);



	matObj->Set("name", string(tempString.C_Str()));
	matObj->Set("diffuse_color", LoadColor(diffuseColor));
	matObj->Set("specular_color", LoadColor(specularColor));
	matObj->Set("ambient_color", LoadColor(ambientColor));


	mat->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTexture);
	mat->GetTexture(aiTextureType_NORMALS, 0, &normalTexture);

	matObj->Set("diffuse_texture", string(diffuseTexture.C_Str()));
	matObj->Set("normal_texture", string(normalTexture.C_Str()));

	return move(matObj);
}

unique_ptr<Object> LoadMesh(vector<Object*>& materials, const aiMesh* mesh, const aiScene* scene)
{
	unique_ptr<Object> meshObj(new Object());

	meshObj->Set("name", string(mesh->mName.C_Str()));

	//enum flags
	//{
	//	HAS_POSITION = 1,
	//	HAS_NORMAL = 2,
	//	HAS_TANGENT_BITANGENT = 4,
	//	HAS_TEXTURE0 = 8,
	//	HAS_COLOR0 = 16,
	//};

	//int f = 0;
	//int vertexSize = 0;
	//float* base;
	//vector<function<void()>> readers;


	//if (mesh->HasPositions())
	//{
	//	f |= HAS_POSITION;
	//	vertexSize += 3;
	//	readers.emplace_back([&base, mesh, &vertexSize](){
	//		for (int i = 0; i < mesh->mNumVertices; i++)
	//		{
	//			copy_n(mesh->mVertices[i], 3, base + i * vertexSize);
	//		}
	//	});
	//}
	//if (mesh->HasNormals())
	//{
	//	f |= HAS_NORMAL;
	//	vertexSize += 3;
	//	readers.emplace_back([&base, mesh, &vertexSize](){
	//		for (int i = 0; i < mesh->mNumVertices; i++)
	//		{
	//			copy_n(mesh->mNormals[i], 3, base + i * vertexSize + 3);
	//		}
	//	});
	//}
	//if (mesh->HasTangentsAndBitangents())
	//{
	//	f |= HAS_TANGENT_BITANGENT;
	//	vertexSize += 6;
	//	readers.emplace_back([&base, mesh, &vertexSize](){
	//		for (int i = 0; i < mesh->mNumVertices; i++)
	//		{
	//			copy_n(mesh->mTangents[i], 3, base + i * vertexSize + 6);
	//			copy_n(mesh->mTangents[i], 3, base + i * vertexSize + 9);
	//		}
	//	});
	//}
	//if (mesh->HasTextureCoords(0))
	//{
	//	f |= HAS_TEXTURE0;
	//	vertexSize += 2;
	//}
	//if (mesh->HasVertexColors(0))
	//{
	//	f |= HAS_COLOR0;
	//	vertexSize += 4;
	//}


	{
		//Yea this is hard coded. Ignore the stuff before this. It's 6AM.
		int vertexSize = 14;
		unique_ptr<TypedArrayObject> vertices(new TypedArrayObject(TypedArrayObject::Type::Float32, vertexSize));
		for (int i = 0; i < mesh->mNumVertices; i++)
		{
			copy_n(&mesh->mVertices[i].x, 3, vertices->Data<float>() + i * vertexSize);

			copy_n(&mesh->mNormals[i].x, 3, vertices->Data<float>() + i * vertexSize + 3);

			copy_n(&mesh->mTangents[i].x, 3, vertices->Data<float>() + i * vertexSize + 6);

			copy_n(&mesh->mBitangents[i].x, 3, vertices->Data<float>() + i * vertexSize + 9);

			copy_n(&mesh->mTextureCoords[0][i].x, 2, vertices->Data<float>() + i * vertexSize + 12);
		}
		meshObj->Set("vertices", move(vertices));
	}

	{
		unique_ptr<TypedArrayObject> indices;

		if (mesh->mNumFaces * 3 > numeric_limits<uint16_t>::max())
		{
			throw runtime_error("Mesh too big! Too many verts.");
		}
		else if (mesh->mNumFaces * 3 > numeric_limits<uint8_t>::max())
		{
			indices.reset(new TypedArrayObject(TypedArrayObject::Type::Uint16, mesh->mNumFaces * 3));
			auto ptr = indices->Data<uint16_t>();
			for (int i = 0; i < mesh->mNumFaces; i++)
			{
				copy_n(mesh->mFaces[i].mIndices, 3, ptr + i * 3);
			}
		}
		else
		{
			indices.reset(new TypedArrayObject(TypedArrayObject::Type::Uint8, mesh->mNumFaces * 3));
			auto ptr = indices->Data<uint8_t>();
			for (int i = 0; i < mesh->mNumFaces; i++)
			{
				copy_n(mesh->mFaces[i].mIndices, 3, ptr + i * 3);
			}
		}

		meshObj->Set("indices", move(indices));

	}

	meshObj->Set("material", materials[mesh->mMaterialIndex]->AssignedTo());

	return move(meshObj);
}



unique_ptr<Object> LoadNode(vector<Object*>& materials, vector<Object*>& meshes, const aiNode* node, const aiScene* scene)
{
	//Todo. This
	//Mergificate models under one node to reduce glVertexAttribPointer calls.
	 
	return nullptr;


}

unique_ptr<Object> LoadScene(const aiScene* scene)
{
	unique_ptr<Object> obj(new Object());	

	vector<Object*> materials;
	vector<Object*> meshes;






}


int main(int argc, const char* args)
{
	fstream file("File.js", fstream::out | fstream::binary);

	Object rootNode;
	rootNode.AssignTo("Root");


	rootNode.Set("Value1", "400");
	rootNode.Set("Value2", 500);
	rootNode.Set("UInt32Array", vector < uint32_t > {1, 2, 3, 4, 5});
	rootNode.Set("UInt16Array", vector < uint16_t > {1, 2, 3, 4, 5});
	rootNode.Set("UInt8Array", vector < uint8_t > {1, 2, 3, 4, 5});
	rootNode.Set("Float32Array", vector < float >(300, 3.0f));

	file << "var " << rootNode.AssignedTo() << " = ";
	rootNode.Print(file);
	file << ";";

	return 0;
}