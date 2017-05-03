/****************************************************************************
* Italian National Research Council                                         *
* Institute for Applied Mathematics and Information Technologies, Genoa     *
* IMATI-GE / CNR                                                            *
*                                                                           *
* Author: Marco Livesu (marco.livesu@gmail.com)                             *
*                                                                           *
* Copyright(C) 2017                                                         *
* All rights reserved.                                                      *
*                                                                           *
* This file is part of CinoLib                                              *
*                                                                           *
* CinoLib is free software; you can redistribute it and/or modify           *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 3 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
****************************************************************************/
#include <cinolib/meshes/polygonmesh/polygonmesh.h>
#include <map>

namespace cinolib
{

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class V_data, class E_data, class F_data>
CINO_INLINE
Polygonmesh<V_data,E_data,F_data>::Polygonmesh(const std::vector<double>            & coords,
                                               const std::vector<std::vector<uint>> & faces)
: coords(coords)
, faces(faces)
{
    init();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class V_data, class E_data, class F_data>
CINO_INLINE
void Polygonmesh<V_data,E_data,F_data>::clear()
{
    bb.reset();
    filename.clear();
    //
    coords.clear();
    edges.clear();
    faces.clear();
    //
    v_data.clear();
    e_data.clear();
    f_data.clear();
    //
    v2v.clear();
    v2e.clear();
    v2f.clear();
    e2f.clear();
    f2e.clear();
    f2f.clear();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class V_data, class E_data, class F_data>
CINO_INLINE
void Polygonmesh<V_data,E_data,F_data>::init()
{
    update_adjacency();
    update_bbox();

    v_data.resize(num_verts());
    e_data.resize(num_edges());
    f_data.resize(num_faces());

    update_normals();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class V_data, class E_data, class F_data>
CINO_INLINE
void Polygonmesh<V_data,E_data,F_data>::update_adjacency()
{
    timer_start("Build adjacency");

    v2v.clear(); v2v.resize(num_verts());
    v2e.clear(); v2e.resize(num_verts());
    v2f.clear(); v2f.resize(num_verts());
    f2f.clear(); f2f.resize(num_faces());
    f2e.clear(); f2e.resize(num_faces());

    std::map<ipair,std::vector<uint>> e2f_map;
    for(uint fid=0; fid<num_faces(); ++fid)
    {
        uint n_sides = faces.at(fid).size();
        assert(n_sides > 2);

        for(uint i=0; i<n_sides; ++i)
        {
            int  vid0 = faces.at(fid).at(i);
            int  vid1 = faces.at(fid).at((i+1)%n_sides);
            v2f.at(vid0).push_back(fid);
            e2f_map[unique_pair(vid0,vid1)].push_back(fid);
        }
    }

    edges.clear();
    e2f.clear();
    e2f.resize(e2f_map.size());

    int fresh_id = 0;
    for(auto e2f_it : e2f_map)
    {
        ipair e    = e2f_it.first;
        int   eid  = fresh_id++;
        int   vid0 = e.first;
        int   vid1 = e.second;

        edges.push_back(vid0);
        edges.push_back(vid1);

        v2v.at(vid0).push_back(vid1);
        v2v.at(vid1).push_back(vid0);

        v2e.at(vid0).push_back(eid);
        v2e.at(vid1).push_back(eid);

        std::vector<uint> fids = e2f_it.second;
        for(int fid : fids)
        {
            f2e.at(fid).push_back(eid);
            e2f.at(eid).push_back(fid);
            for(int adj_fid : fids) if (fid != adj_fid) f2f.at(fid).push_back(adj_fid);
        }

        // MANIFOLDNESS CHECKS
        //
        bool is_manifold = (fids.size() > 2 || fids.size() < 1);
        if (!support_non_manifold_edges)
        {
            std::cerr << "Non manifold edge found! To support non manifoldness,";
            std::cerr << "enable the 'support_non_manifold_edges' flag in cinolib.h" << endl;
            assert(false);
        }
        if (print_non_manifold_edges)
        {
            if (!is_manifold) std::cerr << "Non manifold edge! (" << vid0 << "," << vid1 << ")" << endl;
        }
    }

    logger << num_verts() << "\tverts" << endl;
    logger << num_faces() << "\tfaces" << endl;
    logger << num_edges() << "\tedges" << endl;

    timer_stop("Build adjacency");
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class V_data, class E_data, class F_data>
CINO_INLINE
void Polygonmesh<V_data,E_data,F_data>::update_bbox()
{
    bb.reset();
    for(uint vid=0; vid<num_verts(); ++vid)
    {
        vec3d v = vert(vid);
        bb.min = bb.min.min(v);
        bb.max = bb.max.max(v);
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class V_data, class E_data, class F_data>
CINO_INLINE
void Polygonmesh<V_data,E_data,F_data>::update_f_normals()
{
    for(uint fid=0; fid<num_faces(); ++fid) // TODO: make it more accurate!
    {
        vec3d v0 = face_vert(fid,0);
        vec3d v1 = face_vert(fid,1);
        vec3d v2 = face_vert(fid,2);
        vec3d n  = (v1-v0).cross(v2-v0);
        n.normalize();
        face_data(fid).normal = n;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class V_data, class E_data, class F_data>
CINO_INLINE
void Polygonmesh<V_data,E_data,F_data>::update_v_normals()
{
    for(uint vid=0; vid<num_verts(); ++vid)
    {
        vec3d n(0,0,0);
        for(uint fid : adj_v2f(vid))
        {
            n += face_data(fid).normal;
        }
        n.normalize();
        vert_data(vid).normal = n;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class V_data, class E_data, class F_data>
CINO_INLINE
void Polygonmesh<V_data,E_data,F_data>::update_normals()
{
    update_f_normals();
    update_v_normals();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class V_data, class E_data, class F_data>
CINO_INLINE
vec3d Polygonmesh<V_data,E_data,F_data>::vert(const uint vid) const
{
    uint vid_ptr = vid * 3;
    return vec3d(coords.at(vid_ptr+0), coords.at(vid_ptr+1), coords.at(vid_ptr+2));
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class V_data, class E_data, class F_data>
CINO_INLINE
uint Polygonmesh<V_data,E_data,F_data>::face_vert_id(const uint fid, const uint offset) const
{
    return faces.at(fid).at(offset);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class V_data, class E_data, class F_data>
CINO_INLINE
vec3d Polygonmesh<V_data,E_data,F_data>::face_vert(const uint fid, const uint offset) const
{
    return vert(faces.at(fid).at(offset));
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class V_data, class E_data, class F_data>
CINO_INLINE
vec3d Polygonmesh<V_data,E_data,F_data>::face_centroid(const uint fid) const
{
    vec3d c(0,0,0);
    for(uint vid : faces.at(fid)) c += vert(vid);
    return c;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class V_data, class E_data, class F_data>
CINO_INLINE
uint Polygonmesh<V_data,E_data,F_data>::edge_vert_id(const uint eid, const uint offset) const
{
    uint   eid_ptr = eid * 2;
    return edges.at(eid_ptr + offset);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class V_data, class E_data, class F_data>
CINO_INLINE
vec3d Polygonmesh<V_data,E_data,F_data>::edge_vert(const uint eid, const uint offset) const
{
    uint eid_ptr = eid * 2;
    uint vid     = edges.at(eid_ptr + offset);
    uint vid_ptr = vid * 3;
    return vec3d(coords.at(vid_ptr + 0), coords.at(vid_ptr + 1), coords.at(vid_ptr + 2));
}

}
