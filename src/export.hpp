//
//  export.hpp
//  WebGLJSONExport
//
//  Created by Michael Harris on 8/28/14.
//  Copyright (c) 2014 Michael. All rights reserved.
//

#ifndef WebGLJSONExport_export_hpp
#define WebGLJSONExport_export_hpp

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <exception>
#include <functional>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <numeric>

using namespace std;
using glm::fmat4;
using glm::fvec4;
using glm::fvec3;
using glm::fvec2;

class Material;
class Object;
class Attribute;
class Mesh;
struct json_t;


class BoundingVolume
{
public:
    fvec3 Center;
    fvec3 Extents;
    
    void Union(const BoundingVolume& other);
    void Transform(const fmat4& tran);
    
    inline fvec3 Min() const
    {
        return fvec3(Left(), Bottom(), Near());
    }
    
    inline fvec3 Max() const
    {
        return fvec3(Right(), Top(), Far());
    }
    
    inline float Width() const
    {
        return Extents.x * 2;
    }
    
    inline float Height() const
    {
        return Extents.y * 2;
    }
    
    inline float Depth() const
    {
        return Extents.z * 2;
    }
    
    inline float Left() const
    {
        return Center.x - Extents.x;
    }
    
    inline float Right() const
    {
        return Center.x + Extents.x;
    }
    
    inline float Top() const
    {
        return Center.y + Extents.y;
    }
    
    inline float Bottom() const
    {
        return Center.y - Extents.y;
    }
    
    inline float Near() const
    {
        return Center.z - Extents.z;
    }
    
    inline float Far() const
    {
        return Center.z + Extents.z;
    }
    
    
    json_t* CreateJSON();
};



class Model
{
public:
    vector<unique_ptr<Mesh>> Meshes;
    vector<unique_ptr<Material>> Materials;
    vector<unique_ptr<Object>> Objects;
    vector<unique_ptr<Attribute>> Attributes;
    
    unordered_map<string, Material*> MaterialsMap;
    unordered_map<string, Object*> ObjectsMap;
    unordered_map<string, Attribute*> AttributesMap;
    
    string Name;
    string Data;
    unsigned VertexCount;
    unsigned IndexCount;
    unsigned IndexSize;
    unsigned VertexOffset;
    unsigned IndexOffset;
    unsigned VertexSize;
    
    
    Model(const aiScene* scene);
    
    void Add(unique_ptr<Object>&& obj);
    
    void Add(unique_ptr<Material>&& mat);
    
    void Add(unique_ptr<Mesh>&& mesh);
    
    void Add(unique_ptr<Attribute>&& attr);
    
    long long Write(const vector<float>& vertices);
    
    long long Write(const vector<char>& indices);
    
    json_t* CreateJSON();
    
private:
    vector<float> _Vertices;
    vector<char> _Indices;
};

class Mesh
{
public:
    unsigned long IndexOffset;
    unsigned long IndexCount;
    unsigned long VertexCount;
    unsigned Index;
    BoundingVolume Bounds;
    string Name;
    
    Material* Material;
    
    Mesh(const aiMesh* mesh, Model* model);
    
    json_t* CreateJSON();
};

class Attribute
{
public:
    string Name;
    unsigned Index;
    unsigned Size;
    unsigned Offset;
    
    Attribute() = default;
    Attribute(const Attribute&) = default;
    Attribute(const string& name, unsigned index, unsigned size, unsigned offset)
    : Name(name), Index(index), Offset(offset), Size(size)
    {
        
    }
    
    json_t* CreateJSON();
    
    
    inline bool operator==(const Attribute& o) const
    {
        return Size == o.Size && Offset == o.Offset && Name == o.Name;
    }
};




class Object
{
public:
    string Name;
    vector<Mesh*> Meshes;
    fmat4 Transform;
    BoundingVolume Bounds;
    
    Object(const aiNode* node, Model* model);
    
    json_t* CreateJSON();
};


class Material
{
public:
    unsigned Index;
    string Name;
    string ShadingModel;

    unordered_multimap<string, vector<string>> Textures;
    
    fvec3 AmbientColor;
    fvec3 DiffuseColor;
    fvec3 SpecularColor;
    
    float AmbientCoeff;
    float DiffuseCoeff;
    float SpecularPower;
    float FresnelPower;
    float Roughness;
    
    Material(const aiMaterial* material, Model* model);
    
    json_t* CreateJSON();
};





#endif
