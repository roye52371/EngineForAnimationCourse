// This file is part of libigl, a simple c++ geometry processing library.
//
// Copyright (C) 2014 Daniele Panozzo <daniele.panozzo@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.

#include "Viewer.h"

//#include <chrono>
#include <thread>

#include <Eigen/LU>


#include <cmath>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits>
#include <cassert>

#include <igl/project.h>
//#include <igl/get_seconds.h>
#include <igl/readOBJ.h>
#include <igl/readOFF.h>
#include <igl/adjacency_list.h>
#include <igl/writeOBJ.h>
#include <igl/writeOFF.h>
#include <igl/massmatrix.h>
#include <igl/file_dialog_open.h>
#include <igl/file_dialog_save.h>
#include <igl/quat_mult.h>
#include <igl/axis_angle_to_quat.h>
#include <igl/trackball.h>
#include <igl/two_axis_valuator_fixed_up.h>
#include <igl/snap_to_canonical_view_quat.h>
#include <igl/unproject.h>
#include <igl/serialize.h>
#include <igl\vertex_triangle_adjacency.h>
//Ass1 comment
#include <igl/edge_flaps.h>
#include <igl/shortest_edge_and_midpoint.h>
#include <igl/read_triangle_mesh.h>
#include <igl/opengl/glfw/Viewer.h>
#include <igl/circulation.h>
#include <Eigen/Core>
#include <igl/collapse_edge.h>
#include <set>
//end comment Ass1
//Ass3 include
#include <math.h> 


// Internal global variables used for glfw event handling
//static igl::opengl::glfw::Viewer * __viewer;
static double highdpi = 1;
static double scroll_x = 0;
static double scroll_y = 0;

//Ass1 comment
using namespace std;
using namespace Eigen;
//end comment Ass1

namespace igl
{
namespace opengl
{
namespace glfw
{

  void Viewer::Init(const std::string config)
  {
	  

  }

  IGL_INLINE Viewer::Viewer():
    data_list(1),
    selected_data_index(0),
    next_data_id(1),
	isPicked(false),
	isActive(false),
    link_num(0),
    tip_position(Eigen::RowVector4d(0, 0, 0, 1)),
    destination_position(Eigen::RowVector3d(5,0,0)),//we have 4 zcylinder and 1 sphere
    ikAnimation(false)
  {
    data_list.front().id = 0;

  

    // Temporary variables initialization
   // down = false;
  //  hack_never_moved = true;
    scroll_position = 0.0f;

    // Per face
    data().set_face_based(false);

    
#ifndef IGL_VIEWER_VIEWER_QUIET
    const std::string usage(R"(igl::opengl::glfw::Viewer usage:
  [drag]  Rotate scene
  A,a     Toggle animation (tight draw loop)
  F,f     Toggle face based
  I,i     Toggle invert normals
  L,l     Toggle wireframe
  O,o     Toggle orthographic/perspective projection
  T,t     Toggle filled faces
  [,]     Toggle between cameras
  1,2     Toggle between models
  ;       Toggle vertex labels
  :       Toggle face labels)"
);
    std::cout<<usage<<std::endl;
#endif
  }

  IGL_INLINE Viewer::~Viewer()
  {
      delta = 0.1;
      maxDistance = (data_list.size() - 1) * 1.6;
  }

  IGL_INLINE bool Viewer::load_mesh_from_file(
      const std::string & mesh_file_name_string)
  {

    // Create new data slot and set to selected
    if(!(data().F.rows() == 0  && data().V.rows() == 0))
    {
      append_mesh();
    }
    data().clear();

    size_t last_dot = mesh_file_name_string.rfind('.');
    if (last_dot == std::string::npos)
    {
      std::cerr<<"Error: No file extension found in "<<
        mesh_file_name_string<<std::endl;
      return false;
    }

    std::string extension = mesh_file_name_string.substr(last_dot+1);

    if (extension == "off" || extension =="OFF")
    {
      Eigen::MatrixXd V;
      Eigen::MatrixXi F;
      if (!igl::readOFF(mesh_file_name_string, V, F))
        return false;
      data().set_mesh(V,F);
    }
    else if (extension == "obj" || extension =="OBJ")
    {
      Eigen::MatrixXd corner_normals;
      Eigen::MatrixXi fNormIndices;

      Eigen::MatrixXd UV_V;
      Eigen::MatrixXi UV_F;
      Eigen::MatrixXd V;
      Eigen::MatrixXi F;

      if (!(
            igl::readOBJ(
              mesh_file_name_string,
              V, UV_V, corner_normals, F, UV_F, fNormIndices)))
      {
        return false;
      }

      data().set_mesh(V,F);
      if (UV_V.rows() > 0)
      {
          data().set_uv(UV_V, UV_F);
      }

    }
    else
    {
      // unrecognized file type
      printf("Error: %s is not a recognized file type.\n",extension.c_str());
      return false;
    }

    data().compute_normals();
    data().uniform_colors(Eigen::Vector3d(51.0/255.0,43.0/255.0,33.3/255.0),
                   Eigen::Vector3d(255.0/255.0,228.0/255.0,58.0/255.0),
                   Eigen::Vector3d(255.0/255.0,235.0/255.0,80.0/255.0));

    // Alec: why?
    if (data().V_uv.rows() == 0)
    {
      data().grid_texture();
    }
    

    //for (unsigned int i = 0; i<plugins.size(); ++i)
    //  if (plugins[i]->post_load())
    //    return true;

    //Ass1 comment, use OV, OF as copy to play with the V and F with out touch the original variables
    //init data structure for mesh of every data object
    data().OV = data().V;
    data().OF = data().F;
    initMeshdata();// reset- init data
    //end comment Ass1
    //Ass3
    bool iszcylinder = mesh_file_name_string == "C:/Users/97254/Desktop/run_animation2/Animation3D/tutorial/data/zcylinder.obj";
    //bool iszcylinder = mesh_file_name_string == "C:/Users/roi52/Desktop/ThreeDAnimationCourse/EngineForAnimationCourse/tutorial/data/zcylinder.obj";
    bool first_link_num = link_num == 0;
    if (first_link_num && iszcylinder) {
        data().MyTranslateInSystem(data().GetRotation(), Eigen::RowVector3d(0, 0, 0.8));
        data().tree.init(data().V, data().F);
        data().draw_xyzAxis(data().tree.m_box);
        data().SetCenterOfRotation(Eigen::RowVector3d(0, 0, -0.8));
        /*data().show_overlay_depth = true;
        data().show_overlay = true;
        data().show_texture = true;
        data().set_visible(false, 1);*/
        link_num++;
    }
    else if (!first_link_num && iszcylinder) {
        data().MyTranslateInSystem(data().GetRotation(), Eigen::RowVector3d(0, 0, 1.6));  
        data().tree.init(data().V, data().F);
        data().draw_xyzAxis(data().tree.m_box);
        data().SetCenterOfRotation(Eigen::RowVector3d(0, 0, -0.8));
        /*data().show_overlay_depth = true;
        data().show_overlay = true;
        data().show_texture = true;
        data().set_visible(false, 1);*/
        link_num++;
    }
    //end Ass3
    return true;
  }

  IGL_INLINE bool Viewer::save_mesh_to_file(
      const std::string & mesh_file_name_string)
  {
    // first try to load it with a plugin
    //for (unsigned int i = 0; i<plugins.size(); ++i)
    //  if (plugins[i]->save(mesh_file_name_string))
    //    return true;

    size_t last_dot = mesh_file_name_string.rfind('.');
    if (last_dot == std::string::npos)
    {
      // No file type determined
      std::cerr<<"Error: No file extension found in "<<
        mesh_file_name_string<<std::endl;
      return false;
    }
    std::string extension = mesh_file_name_string.substr(last_dot+1);
    if (extension == "off" || extension =="OFF")
    {
      return igl::writeOFF(
        mesh_file_name_string,data().V,data().F);
    }
    else if (extension == "obj" || extension =="OBJ")
    {
      Eigen::MatrixXd corner_normals;
      Eigen::MatrixXi fNormIndices;

      Eigen::MatrixXd UV_V;
      Eigen::MatrixXi UV_F;

      return igl::writeOBJ(mesh_file_name_string,
          data().V,
          data().F,
          corner_normals, fNormIndices, UV_V, UV_F);
    }
    else
    {
      // unrecognized file type
      printf("Error: %s is not a recognized file type.\n",extension.c_str());
      return false;
    }
    return true;
  }
 
  IGL_INLINE bool Viewer::load_scene()
  {
    std::string fname = igl::file_dialog_open();
    if(fname.length() == 0)
      return false;
    return load_scene(fname);
  }

  IGL_INLINE bool Viewer::load_scene(std::string fname)
  {
   // igl::deserialize(core(),"Core",fname.c_str());
    igl::deserialize(data(),"Data",fname.c_str());
    return true;
  }

  IGL_INLINE bool Viewer::save_scene()
  {
    std::string fname = igl::file_dialog_save();
    if (fname.length() == 0)
      return false;
    return save_scene(fname);
  }

  IGL_INLINE bool Viewer::save_scene(std::string fname)
  {
    //igl::serialize(core(),"Core",fname.c_str(),true);
    igl::serialize(data(),"Data",fname.c_str());

    return true;
  }

  IGL_INLINE void Viewer::open_dialog_load_mesh()
  {
      /*    //Ass 3 comment

    std::string fname = igl::file_dialog_open();

    if (fname.length() == 0)
      return;*/
    //this->load_mesh_from_file(fname.c_str());
    this->load_mesh_from_file("C:/Users/97254/Desktop/run_animation2/Animation3D/tutorial/data/zcylinder.obj");
    //this->load_mesh_from_file("C:/Users/roi52/Desktop/ThreeDAnimationCourse/EngineForAnimationCourse/tutorial/data/zcylinder.obj");

    //end comment Ass 3
  }

  IGL_INLINE void Viewer::open_dialog_save_mesh()
  {
    std::string fname = igl::file_dialog_save();

    if(fname.length() == 0)
      return;

    this->save_mesh_to_file(fname.c_str());
  }

  IGL_INLINE ViewerData& Viewer::data(int mesh_id /*= -1*/)
  {
    assert(!data_list.empty() && "data_list should never be empty");
    int index;
    if (mesh_id == -1)
      index = selected_data_index;
    else
      index = mesh_index(mesh_id);

    assert((index >= 0 && index < data_list.size()) &&
      "selected_data_index or mesh_id should be in bounds");
    return data_list[index];
  }

  IGL_INLINE const ViewerData& Viewer::data(int mesh_id /*= -1*/) const
  {
    assert(!data_list.empty() && "data_list should never be empty");
    int index;
    if (mesh_id == -1)
      index = selected_data_index;
    else
      index = mesh_index(mesh_id);

    assert((index >= 0 && index < data_list.size()) &&
      "selected_data_index or mesh_id should be in bounds");
    return data_list[index];
  }

  IGL_INLINE int Viewer::append_mesh(bool visible /*= true*/)
  {
    assert(data_list.size() >= 1);

    data_list.emplace_back();
    selected_data_index = data_list.size()-1;
    data_list.back().id = next_data_id++;
    //if (visible)
    //    for (int i = 0; i < core_list.size(); i++)
    //        data_list.back().set_visible(true, core_list[i].id);
    //else
    //    data_list.back().is_visible = 0;
    return data_list.back().id;
  }

  IGL_INLINE bool Viewer::erase_mesh(const size_t index)
  {
    assert((index >= 0 && index < data_list.size()) && "index should be in bounds");
    assert(data_list.size() >= 1);
    if(data_list.size() == 1)
    {
      // Cannot remove last mesh
      return false;
    }
    data_list[index].meshgl.free();
    data_list.erase(data_list.begin() + index);
    if(selected_data_index >= index && selected_data_index > 0)
    {
      selected_data_index--;
    }

    return true;
  }

  IGL_INLINE size_t Viewer::mesh_index(const int id) const {
    for (size_t i = 0; i < data_list.size(); ++i)
    {
      if (data_list[i].id == id)
        return i;
    }
    return 0;
  }

  Eigen::Matrix4d Viewer::CalcParentsTrans(int indx) 
  {
	  Eigen::Matrix4d prevTrans = Eigen::Matrix4d::Identity();

	  for (int i = indx; parents[i] >= 0; i = parents[i])
	  {
		  //std::cout << "parent matrix:\n" << scn->data_list[scn->parents[i]].MakeTrans() << std::endl;
		  prevTrans = data_list[parents[i]].MakeTransd() * prevTrans;
	  }

	  return prevTrans;
  }



  //Ass1 comment - reset - init data

  void Viewer::comp_obj_quad_error() {
      
      Eigen::MatrixXd V = data().OV;
      Eigen::MatrixXi F = data().OF;
      std::vector<std::vector<int> > VF;
      std::vector<std::vector<int> > VFi;

      igl::vertex_triangle_adjacency(V, F, VF, VFi);//update adjacency list for each vertex which contains its adjacent faces

      for (int vi = 0; vi < V.rows(); vi++) {
          //going over on all of the vertexes of curr mesh
          // find edges with this vertex
          std::vector<int> faces;
          data().Quads[vi] = Eigen::Matrix4d::Zero();//initializing Quads with 0 matrix before giving values

          for (int fj = 0; fj < VF[vi].size(); fj++) {
              Eigen::Vector3d norm = data().F_normals.row(VF[vi][fj]).normalized();//stage 10
              //VF[vi][fi] take the j face of the i vertex, compute its normal
              double d = V.row(vi) * norm;
              double a = norm[0], b = norm[1], c = norm[2];
              d *= -1;

              Eigen::Matrix4d Kp;
              Kp.row(0) = Eigen::Vector4d(a * a, a * b, a * c, a * d);
              Kp.row(1) = Eigen::Vector4d(a * b, b * b, b * c, b * d);
              Kp.row(2) = Eigen::Vector4d(a * c, b * c, c * c, c * d);
              Kp.row(3) = Eigen::Vector4d(a * d, d * b, d * c, d * d);

              data().Quads[vi] += Kp;
          }
      }
  }
  void Viewer:: comp_opt_cost_position(const int e, const Eigen::MatrixXd& V, double& cost, Eigen::Vector3d& p)
  {
      
      //printf("first line in comp_opt_cost_position\n");
      int v1_index = data().E(e,0);
      int v2_index = data().E(e, 1);
      Eigen::Matrix4d q12 = data().Quads[v1_index] + data().Quads[v2_index];//Q = Q1 + Q2
      Eigen::Matrix4d qtag = q12;
      qtag.row(3) = Eigen::Vector4d(0, 0, 0, 1);//4th row vector
      bool invert;
      Eigen::Vector4d::Scalar det;
      double a;
      qtag.computeInverseAndDetWithCheck(qtag, det, invert, a);//inverse,determinant,invertible_bool,Threshold
      Eigen::Vector4d p_cost;//declaring 4d vector for cost
      //location of new vertex
      if (invert) {
          //minimum quadric error
          caseInverible(p_cost, qtag, p);
      }
      else {
          //avg_case
          caseNotInvertible(p, V, v1_index, v2_index, p_cost);
      }
      cost = formula_cost(p_cost, q12);
  }
  void Viewer::caseInverible(Eigen::Vector4d& p1, Eigen::Matrix4d& qtag, Eigen::Vector3d& p2) {
      p1 = qtag * (Eigen::Vector4d(0, 0, 0, 1));
      p2[0] = p1[0];
      p2[1] = p1[1];
      p2[2] = p1[2];
  }
  void Viewer::caseNotInvertible(Eigen::Vector3d& p1, const Eigen::MatrixXd& V, int v1, int v2, Eigen::Vector4d& p2) {
      p1 = (V.row(v1) + V.row(v2)) / 2;
      p2[0] = p1[0];
      p2[1] = p1[1];
      p2[2] = p1[2];
      p2[3] = 1;
  }
  double Viewer::formula_cost(Eigen::Vector4d p_cost, Eigen::Matrix4d q12) {
      return p_cost.transpose() * q12 * p_cost;
  }
  bool Viewer::new_collapse_edge(Eigen::MatrixXd& V, Eigen::MatrixXi& F) {
      if (data().Q->empty())
      {
          //no more edges in the priority q
          return false;
      }
      //stage a
      std::pair<double, int> p = *(data().Q->begin());//pop the edge to collapse by Q elements order
      if (p.first == std::numeric_limits<double>::infinity())
      {
          //no more edges to remove cause minimum has infinite cost
          return false;
      }
      data().Q->erase(data().Q->begin());//erasing the minimum cost edge

      int e = p.second;//edge index

      (*data().Qit)[e] = data().Q->end();//reset delete the removed edge iterator
      
      std::vector<int> neighbor_faces = circulation(e, true, data().EMAP, data().EF, data().EI);//Return list of faces around the end point of an edge e(in direction of e1 when e = (e1,e2))
      std::vector<int> neighbor_opposite_dir = circulation(e, false, data().EMAP, data().EF, data().EI);//Return list of faces the around the end point of an edge e (in direction of e2 when e = (e1,e2))
      neighbor_faces.insert(neighbor_faces.begin(), neighbor_opposite_dir.begin(), neighbor_opposite_dir.end());//inserting all faces the around the end point of an edge e to one vector N(insert ND to the begin of N)

      int v1_index = data().E.row(e)[0];
      int v2_index = data().E.row(e)[1];

      int e1, e2, f1, f2;
      //stage b + c + d of 11 is written by the library function collapse_edge
      bool collapsed = igl::collapse_edge(e, data().C.row(e), V, F, data().E, data().EMAP, data().EF, data().EI, e1, e2, f1, f2);//If valid, then parameters are adjusted accordingly(703example)

      if (collapsed)
      {
          //stage e
          // update the quad of the new vertex		
          data().Quads[v1_index] = data().Quads[v1_index] + data().Quads[v2_index];
          data().Quads[v2_index] = data().Quads[v1_index];

          // delete the others collapse edges
          (*data().Q).erase((*data().Qit)[e1]);
          (*data().Qit)[e1] = (*data().Q).end();
          (*data().Q).erase((*data().Qit)[e2]);
          (*data().Qit)[e2] = (*data().Q).end();
          // update local neighbors
          // loop over original face neighbors
          for (auto neighbor_index : neighbor_faces)
          {
                for (int v_index = 0; v_index < 3 && check_existance(neighbor_index, F); v_index++)
                //updating all neighbores of the deleted edge
                {
                    // edge index
                    int e_index = data().EMAP(v_index * F.rows() + neighbor_index);
                    // delete not updated cost
                    (*data().Q).erase((*data().Qit)[e_index]);
                    double cost;
                    Vector3d newpos;
                    //computing the optimal cost and new position
                    comp_opt_cost_position(e_index, V, cost, newpos);
                    // insert updated cost to Q and updating the iterator
                    (*data().Qit)[e_index] = (*data().Q).insert(std::pair<double, int>(cost, e_index)).first;
                    data().C.row(e_index) = newpos;
                }
              
          }
          
          //stage f
          Vector3d v_newpos = data().C.row(e);
          std::cout << "edge " << e << ", cost = " << p.first << ", new v position (" << v_newpos[0] << "," << v_newpos[1] << "," << v_newpos[2] << ")" << endl;
          
      }
      else
      {
          //insert infinite weight if not collapsed
          p.first = std::numeric_limits<double>::infinity();
          (*data().Qit)[e] = (*data().Q).insert(p).first;
      }
      //return if something collapsed
      return collapsed;
  }
  bool Viewer::check_existance(int neighbor_index, Eigen::MatrixXi &F) {
      //checking for original edge of face neighbor existance
      return ((F(neighbor_index, 0) != IGL_COLLAPSE_EDGE_NULL) ||
          (F(neighbor_index, 1) != IGL_COLLAPSE_EDGE_NULL) ||
          (F(neighbor_index, 2) != IGL_COLLAPSE_EDGE_NULL));
  }
  void Viewer::initMeshdata() {
      
      Eigen::MatrixXi F = data().OF;
      Eigen::MatrixXd V = data().OV;
      Eigen::VectorXi EMAP;
      Eigen::MatrixXi E, EF, EI;
      Eigen::MatrixXd C;
      PriorityQueue* Q = new PriorityQueue;
      std::vector<PriorityQueue::iterator >* Qit = new std::vector<PriorityQueue::iterator >;// keep the iterators of edges of Q
      //so we can interate and find the neighbor edges of the edge e (lets say that we iteratte using edge e's iterator )

      //second part Ass1
      data().Quads.resize(V.rows());

      //
      data().F = F;
      data().V = V;
      data().E = E;
      data().EF = EF;
      data().EI = EI;
      data().EMAP = EMAP;
      data().Q = Q;
      data().Qit = Qit;
      data().C = C;
      //

      comp_obj_quad_error();
      //printf("after quad_error\n");
      //end cimmwnt secind oart Ass

      edge_flaps(data().F, data().E, data().EMAP, data().EF, data().EI); // filing our data struct object EMAP, EF, EI, E and F(edges and faces)
      data().Qit->resize(data().E.rows());
      //resize to giving the current space we need
      data().C.resize(data().E.rows(), data().V.cols()); //keep the new vertices location, for each edge e collapsed, keep the new vertex cordinates he became to
      //cooridate of vertex (example (0,0,1))
      Eigen::VectorXd costs(data().E.rows());
      data().Q->clear();
      //printf("before for loop\n");

      for (int e = 0; e < data().E.rows(); e++)
      {
          double cost = e;

          Eigen::Vector3d p(0,0,0);

          //the function adjust costs and placements of inital data object, using shortest edge size as cost alg, and  midpoint of collapsed edge as new vertex alg
          // shortest_edge_and_midpoint(e, V, F, E, EMAP, EF, EI, cost, p);//algorithm to calc cost in simplification, and return new vertices
          //of collapse edges

          //second oart Ass1
          comp_opt_cost_position(e, data().V, cost, p);//algorithm to calc cost in simplification
          //end cimmwnt secind oart Ass


          data().C.row(e) = p;//keep new vertex created from edge collapsion
          (*data().Qit)[e] = data().Q->insert(std::pair<double, int>(cost, e)).first;//keep e's iterator
      }
      data().set_mesh(data().V, data().F);     
  }

  void Viewer::meshSimplification(double num_iter) {
      // simplifying the mesh
      bool something_collapsed = false;
      // collapse edge
      const int max_iter = std::ceil(num_iter);//max edge collapse num_iter each time as asked
      for (int j = 0; j < max_iter; j++)
      {
          //this collapse edge, takes:
          //priority queue based edge collapse with function handles to adjust costs and placements,
          //and all other data,
          //and collapsed edge accordingly and adjust cost and placements according to the collapse
          //if (!collapse_edge(shortest_edge_and_midpoint, data().V, data().F, data().E, data().EMAP, data().EF, data().EI, *(data().Q), *(data().Qit), data().C))
          if (!new_collapse_edge(data().V,data().F))
          {
              break;
          }
          something_collapsed = true;
      }

      if (something_collapsed)
      {
          //if something collapsed need to update the mesh of our object data to be with less triangles and current vertices and faces
          Eigen::MatrixXd V = data().V;
          Eigen::MatrixXi F = data().F;
          //line asked to be added in Assignment
          data().clear();
          data().set_mesh(V, F);
          data().set_face_based(true);
          data().dirty = 157;
          //end of comment -line asked to be added in Assignment
      }

      //return false;
  }
  //end comment Ass1


  //Ass 2 comment
  void Viewer:: initTreesAndDrawForCollision() {
      isActive = false;//make it false at the begining, so we cam control when to start the collision simulation
      //moving the scene and the object, for start of collision simulation
      MyTranslate(Eigen::Vector3d(0, 0, -0.4), true);//for seening the object smaller so we have space to move more
      data_list[0].MyTranslate(Eigen::Vector3d(0.65, 0, 0), true);//moving the objects so they won't be on each other at initial running time
      data_list[1].MyTranslate(Eigen::Vector3d(-0.65, 0, 0), true);
      /*viewer.data_list[0].show_overlay = 1;
      viewer.data_list[1].show_overlay = 1;
      viewer.data_list[0].show_overlay_depth = 1;
      viewer.data_list[1].show_overlay_depth = 1;*/


      //init the tree of both objects, and draw their bounding box

      data_list[0].tree.init(data_list[0].V, data_list[0].F);
      igl::AABB<Eigen::MatrixXd, 3> tree_first = data_list[0].tree;
      Eigen::AlignedBox<double, 3> box_first = tree_first.m_box;
      data_list[0].drawBox(box_first, 0);

      data_list[1].tree.init(data_list[1].V, data_list[1].F);
      igl::AABB<Eigen::MatrixXd, 3> tree_second = data_list[1].tree;
      Eigen::AlignedBox<double, 3> box_second = tree_second.m_box;
      data_list[1].drawBox(box_second, 0);
  }



  void Viewer::setMovingButton() {
      isActive = !isActive;
  }

  using namespace std;

  //Returns true if box 1 and box 2 collide
  bool Viewer::checkTermsForBoxesCollision(Eigen::AlignedBox<double, 3>& box1, Eigen::AlignedBox<double, 3>& box2) {
      double R, R0, R1;

      //parameters from page 35 of "Separating Axis Theorem for Oriented Bounding Boxes"
      //A parameters
      Eigen::RowVector3d Ax = data_list[0].GetRotation() * Eigen::Vector3d(1, 0, 0);
      Eigen::RowVector3d Ay = data_list[0].GetRotation() * Eigen::Vector3d(0, 1, 0);
      Eigen::RowVector3d Az = data_list[0].GetRotation() * Eigen::Vector3d(0, 0, 1);
      Eigen::Matrix3d A;
      A << Ax[0], Ay[0], Az[0],
          Ax[1], Ay[1], Az[1],
          Ax[2], Ay[2], Az[2];
      double W_A = box1.sizes()[0] / 2;//half width of A
      double H_A = box1.sizes()[1] / 2;//half height of A
      double D_A = box1.sizes()[2] / 2;//half depth of A

      //B parameters
      Eigen::RowVector3d Bx = data_list[1].GetRotation() * Eigen::Vector3d(1, 0, 0);
      Eigen::RowVector3d By = data_list[1].GetRotation() * Eigen::Vector3d(0, 1, 0);
      Eigen::RowVector3d Bz = data_list[1].GetRotation() * Eigen::Vector3d(0, 0, 1);
      Eigen::Matrix3d B;
      B << Bx[0], By[0], Bz[0],
          Bx[1], By[1], Bz[1],
          Bx[2], By[2], Bz[2];
      double W_B = box2.sizes()[0] / 2;//half width of B
      double H_B = box2.sizes()[1] / 2;//half height of B
      double D_B = box2.sizes()[2] / 2;//half depth of B


      Eigen::Matrix3d Rij = A.transpose() * B;
      Eigen::Vector4f tmp1 = Eigen::Vector4f(box1.center()[0], box1.center()[1], box1.center()[2], 1);
      tmp1 = data_list[0].MakeTransScale() * tmp1;
      Eigen::Vector3d P_A = Eigen::Vector3d(tmp1[0], tmp1[1], tmp1[2]);//coordinate position of the center of A

      Eigen::Vector4f tmp2 = Eigen::Vector4f(box2.center()[0], box2.center()[1], box2.center()[2], 1);
      tmp2 = data_list[1].MakeTransScale() * tmp2;
      Eigen::Vector3d P_B = Eigen::Vector3d(tmp2[0], tmp2[1], tmp2[2]);//coordinate position of the center of B

      Eigen::Vector3d T = P_B - P_A;
      //ALL CASES WERE TAKEN FROM "Separating Axis Theorem for Oriented Bounding Boxes"
      //OPTIMIZED VERSION PAGE 35 -37
      // Dot product = mahkpela scalarit
      // CASE 1: Ax
      R0 = W_A;
      R1 = W_B * abs(Rij(0, 0)) + H_B * abs(Rij(0, 1)) + D_B * abs(Rij(0, 2));
      R = abs(Ax.dot(T));
      if (R > R0 + R1)
          return false;
      // CASE 2: Ay
      R0 = H_A;
      R1 = W_B * abs(Rij(1, 0)) + H_B * abs(Rij(1, 1)) + D_B * abs(Rij(1, 2));
      R = abs(Ay.dot(T));
      if (R > R0 + R1)
          return false;
      // CASE 3: Az
      R0 = D_A;
      R1 = W_B * abs(Rij(2, 0)) + H_B * abs(Rij(2, 1)) + D_B * abs(Rij(2, 2));
      R = abs(Az.dot(T));
      if (R > R0 + R1)
          return false;
      // CASE 4: Bx
      R0 = W_A * abs(Rij(0, 0)) + H_A * abs(Rij(1, 0)) + D_A * abs(Rij(2, 0));
      R1 = W_B;
      R = abs(Bx.dot(T));
      if (R > R0 + R1)
          return false;
      // CASE 5: By
      R0 = W_A * abs(Rij(0, 1)) + H_A * abs(Rij(1, 1)) + D_A * abs(Rij(2, 1));
      R1 = H_B;
      R = abs(By.dot(T));
      if (R > R0 + R1)
          return false;
      // CASE 6: Bz
      R0 = W_A * abs(Rij(0, 2)) + H_A * abs(Rij(1, 2)) + D_A * abs(Rij(2, 2));
      R1 = D_B;
      R = abs(Bz.dot(T));
      if (R > R0 + R1)
          return false;
      // CASE 7: Ax * Bx
      R0 = H_A * abs(Rij(2, 0)) + D_A * abs(Rij(1, 0));
      R1 = H_B * abs(Rij(0, 2)) + D_B * abs(Rij(0, 1));
      R = abs(Rij(1, 0) * Az.dot(T) - Rij(2, 0) * Ay.dot(T));
      if (R > R0 + R1)
          return false;
      // CASE 8: Ax * By
      R0 = H_A * abs(Rij(2, 1)) + D_A * abs(Rij(1, 1));
      R1 = W_B * abs(Rij(0, 2)) + D_B * abs(Rij(0, 0));
      R = abs(Rij(1, 1) * Az.dot(T) - Rij(2, 1) * Ay.dot(T));
      if (R > R0 + R1)
          return false;
      // CASE 9: Ax * Bz
      R0 = H_A * abs(Rij(2, 2)) + D_A * abs(Rij(1, 2));
      R1 = W_B * abs(Rij(0, 1)) + H_B * abs(Rij(0, 0));
      R = abs(Rij(1, 2) * Az.dot(T) - Rij(2, 2) * Ay.dot(T));
      if (R > R0 + R1)
          return false;
      // CASE 10: Ay * Bx
      R0 = W_A * abs(Rij(2, 0)) + D_A * abs(Rij(0, 0));
      R1 = H_B * abs(Rij(1, 2)) + D_B * abs(Rij(1, 1));
      R = abs(Rij(2, 0) * Ax.dot(T) - Rij(0, 0) * Az.dot(T));
      if (R > R0 + R1)
          return false;
      // CASE 11: Ay * By
      R0 = W_A * abs(Rij(2, 1)) + D_A * abs(Rij(0, 1));
      R1 = W_B * abs(Rij(1, 2)) + D_B * abs(Rij(1, 0));
      R = abs(Rij(2, 1) * Ax.dot(T) - Rij(0, 1) * Az.dot(T));
      if (R > R0 + R1)
          return false;
      // CASE 12: Ay * Bz
      R0 = W_A * abs(Rij(2, 2)) + D_A * abs(Rij(0, 2));
      R1 = W_B * abs(Rij(1, 1)) + H_B * abs(Rij(1, 0));
      R = abs(Rij(2, 2) * Ax.dot(T) - Rij(0, 2) * Az.dot(T));
      if (R > R0 + R1)
          return false;
      // CASE 13: Az * Bx
      R0 = W_A * abs(Rij(1, 0)) + H_A * abs(Rij(0, 0));
      R1 = H_B * abs(Rij(2, 2)) + D_B * abs(Rij(2, 1));
      R = abs(Rij(0, 0) * Ay.dot(T) - Rij(1, 0) * Ax.dot(T));
      if (R > R0 + R1)
          return false;
      // CASE 14: Az * By
      R0 = W_A * abs(Rij(1, 1)) + H_A * abs(Rij(0, 1));
      R1 = W_B * abs(Rij(2, 2)) + D_B * abs(Rij(2, 0));
      R = abs(Rij(0, 1) * Ay.dot(T) - Rij(1, 1) * Ax.dot(T));
      if (R > R0 + R1)
          return false;
      // CASE 15: Az * Bz
      R0 = W_A * abs(Rij(1, 2)) + H_A * abs(Rij(0, 2));
      R1 = W_B * abs(Rij(2, 1)) + H_B * abs(Rij(2, 0));
      R = abs(Rij(0, 2) * Ay.dot(T) - Rij(1, 2) * Ax.dot(T));
      if (R > R0 + R1)
          return false;
      return true;

  }

  //Recursion call for checking collision, retruns true if node1 and node2 collide (checking untill leafs recursivly)
  //If they collide, draw the box of the leaf in each data items
  bool Viewer::recursiveCheckCollision(igl::AABB<Eigen::MatrixXd, 3>* node1, igl::AABB<Eigen::MatrixXd, 3>* node2) {
      if (checkTermsForBoxesCollision(node1->m_box, node2->m_box))
      {
          //No children, this is a leaf, drawing the box
          if (node1->is_leaf() && node2->is_leaf())
          {
              data_list[0].drawBox(node1->m_box, 1);
              data_list[1].drawBox(node2->m_box, 1);
              return true;
          }
          else {
              //Children pointers
              // m_left and m_right are shared pointers in AABB class
              igl::AABB<Eigen::MatrixXd, 3>* n1_left = node1->is_leaf() ? node1 : node1->m_left;
              igl::AABB<Eigen::MatrixXd, 3>* n2_left = node2->is_leaf() ? node2 : node2->m_left;

              igl::AABB<Eigen::MatrixXd, 3>* n1_right = node1->is_leaf() ? node1 : node1->m_right;
              igl::AABB<Eigen::MatrixXd, 3>* n2_right = node2->is_leaf() ? node2 : node2->m_right;

              //looking for every type of intersection between the children's node of each object node
              if (recursiveCheckCollision(n1_left, n2_left) ||
                  recursiveCheckCollision(n1_left, n2_right) ||
                  recursiveCheckCollision(n1_right, n2_left) ||
                  recursiveCheckCollision(n1_right, n2_right))
                  return true;
              else
                  return false;
          }
      }
      else
          return false;
  }

  void Viewer::checkCollision() {
      if (recursiveCheckCollision(&data_list[0].tree, &data_list[1].tree)) {
          cout << "Objects had a collision !" << endl;
          isActive = false;
      }
  }
  //end comment Ass 2


  //Ass3 comment
  void Viewer::toggleIKSimulation() {
      if (isActive == true) {
          //return;
          axisFixer();
      }
      isActive = !isActive;
  }

  void Viewer::IKSimulation() {

      Eigen::Vector4d fstCylin4 = data_list[1].MakeTransd() * Eigen::Vector4d(0,0, -0.8, 1);
      Eigen::Vector3d fstCylin = Eigen::Vector3d(fstCylin4[0], fstCylin4[1], fstCylin4[2]);

      Eigen::Vector4d sphere4 = data_list[0].MakeTransd() * Eigen::Vector4d(0, 0, 0, 1);
      Eigen::Vector3d sphere = Eigen::Vector3d(sphere4[0], sphere4[1], sphere4[2]);

      double dist = (fstCylin - sphere).norm();

      //if (dist > 6.4) { //6.4 is arm length fully extended
      if (dist > (data_list.size()-1)*1.6) { // its arm length fully extended(with out taking the sphere)
          //cout<<data_list.size() - 2<<endl;
          cout << "cannot reach" << endl;
          isActive = false;
          return;
      }
      Eigen::Vector4d E4;
      Eigen::Vector3d E;
      for (int i = data_list.size() - 1; i > 0; i--) {
          //E4 = ParentsTrans_mat4d(4) * data_list[4].MakeTransd() * Eigen::Vector4d(0, 0.8, 0, 1);
          E4 = ParentsTrans_mat4d(data_list.size() - 1) * data_list[data_list.size() - 1].MakeTransd() * Eigen::Vector4d(0,0, 0.8, 1);
          E = Eigen::Vector3d(E4[0], E4[1], E4[2]);
          dist = (E - sphere).norm();

          Eigen::Vector4d R4 = ParentsTrans_mat4d(i) * data_list[i].MakeTransd() * Eigen::Vector4d(0,0, -0.8, 1);
          Eigen::Vector3d R = Eigen::Vector3d(R4[0], R4[1], R4[2]);

          Eigen::Vector3d RE = E - R;
          Eigen::Vector3d RD = sphere - R;

          double dot = RD.normalized().dot(RE.normalized());
          double aDegRad = acos (dot); //alpha in radians
          if (dist > 0.3)
              aDegRad = aDegRad / 20;
          if (dot >= 1.0)//checking for legal value for acos
              aDegRad = 0;

          Eigen::Vector3d cross_product = RE.cross(RD);
          cross_product.normalize();
          cross_product = ParentsInvRot_mat3d(i) * cross_product;
          data_list[i].MyRotate(cross_product, aDegRad, false);
      }
      E4 = ParentsTrans_mat4d(data_list.size() - 1) * data_list[data_list.size() - 1].MakeTransd() * Eigen::Vector4d(0,0, 0.8, 1);
      E = Eigen::Vector3d(E4[0], E4[1], E4[2]);
      dist = (E - sphere).norm();
      if (dist < 0.1 || isActive == false) {
          isActive = false;
          axisFixer();
      }
  }
  
  void Viewer::axisFixer() {

      for (int i = 1; i <= data_list.size() - 1; i++) {
          Eigen::Matrix3d rotation_mat = data_list[i].GetRotation();
          cout << "RU:\n" << rotation_mat << endl;
          if (rotation_mat(2, 2) < 1.0) {
              if (rotation_mat(2, 2) > -1.0) {
                  double z = atan2(rotation_mat(2, 0), -rotation_mat(2, 1));
                  data_list[i].MyRotate(Eigen::Vector3d(0, 0, 1), -z, false);
                  if(i!= data_list.size()-1){
                      data_list[i + 1].MyRotate(Eigen::Vector3d(0, 0, 1), z, true);
                  }
              }
          }
      }
  }

  Eigen::Matrix4d Viewer::ParentsTrans_mat4d(int index) {
      if(index <= 1)
          return Transform<double, 3, Eigen::Affine>::Identity().matrix();
      return ParentsTrans_mat4d(index - 1) * data_list[index - 1].MakeTransd();
  }

  Eigen::Matrix3d Viewer::ParentsInvRot_mat3d(int index) {
      Matrix3d inv_rot = data(index).GetRotation().inverse();
      for (int i = index - 1; i > 0; i--) 
          inv_rot = inv_rot * data(i).GetRotation().inverse();
      return inv_rot;
  }

  ///////////////////////////////

  Eigen::Matrix4d Viewer::maketranslatetoshape(int indx){
		return data_list[indx].MakeTransd();
  }
  
  void Viewer::calculateStep(std::vector<Eigen::Vector3d> before, std::vector<Eigen::Vector3d> points)
  {
      Eigen::Vector4d E4;
      Eigen::Vector3d E;
      Eigen::Vector3d Zaxis;
      Eigen::Vector3d Xaxis;
      Eigen::Vector3d Vp;
      Eigen::Vector3d V;

      Eigen::Vector3d a, b, newZ;
      double theta, phi, cosPhi, cosTheta, sinPhi, sinTheta;
      Eigen::Vector4d root4 = data_list[1].MakeTransd() * Eigen::Vector4d(0, 0, -0.8, 1);
      Eigen::Vector3d root = Eigen::Vector3d(root4[0], root4[1], root4[2]);

      Eigen::Vector4d ball4 = data_list[0].MakeTransd() * Eigen::Vector4d(0, 0, 0, 1);
      Eigen::Vector3d ball = Eigen::Vector3d(ball4[0], ball4[1], ball4[2]);

      double dist = (root - ball).norm();

      //if (dist > 6.4) { //6.4 is arm length fully extended
      if (dist > (data_list.size() - 1) * 1.6) { // its arm length fully extended(with out taking the sphere)
          //cout<<data_list.size() - 2<<endl;
          cout << "cannot reach" << endl;
          isActive = false;
          return;
      }



      int linknum = data_list.size() - 1;
      for (int i = 1; i < linknum; i++) {
          double signX = 1;
          double signZ = 1;
          if (before.at(data_list.size())(0) - points.at(1)(0) > 0)
              signX = -1;
          double r, gamma;;
          Eigen::Vector3d Zaxis;
          Eigen::Vector3d Xaxis;

          Eigen::Matrix4d axiesMat = Eigen::Matrix4d::Ones();
          for (int j = 0; j < i; j++)
          {
              axiesMat = axiesMat * maketranslatetoshape(j);
          }
           
          Xaxis = Eigen::Vector3d(axiesMat.row(0)(1),axiesMat.row(0)(1),axiesMat.row(0)(2));
          Zaxis = Eigen::Vector3d(axiesMat.row(2)(1),axiesMat.row(2)(1),axiesMat.row(2)(2));


          Eigen::Vector3d Vp = points[i + 1] - points[i];
          Zaxis.normalize();
          Xaxis.normalize();
          if (Vp != Eigen::Vector3d(0, 0, 0))
              Vp.normalize();
          Eigen::Vector3d V = Zaxis.cross(Vp);
          if (V != Eigen::Vector3d(0, 0, 0))
              V.normalize();

            
          double theta = acos(Zaxis.dot(Vp));
          theta = theta < -1.0 ? -1.0 : theta;
          theta = theta > 1.0 ? 1.0 : theta; //acos(clamp(Zaxis.dot(Vp), -1.0, 1.0));

          double phi =  -acos(Xaxis.dot(V));
          phi = phi < -1.0 ? -1.0 : phi;
          phi = phi > 1.0 ? 1.0 : phi; //-acos(clamp(Xaxis.dot(V), -1.0, 1.0));




          double alphaRad = theta; //alpah in radians
          if (dist > 0.3)
              alphaRad = alphaRad / 20;

           data_list[i].MyRotate(Zaxis, alphaRad, true);

           alphaRad = phi; //alpah in radians
          if (dist > 0.3)
              alphaRad = alphaRad / 20;

          data_list[i].MyRotate(Xaxis, alphaRad, false);
          /*
          alphaRad = theta; //alpah in radians
          if (dist > 0.3)
              alphaRad = alphaRad / 20;

           data_list[i].MyRotate(Zaxis, alphaRad, true);
           */
          E4 = ParentsTrans_mat4d(data_list.size() - 1) * data_list[data_list.size() - 1].MakeTransd() * Eigen::Vector4d(0,0, 0.8, 1);
          E = Eigen::Vector3d(E4[0], E4[1], E4[2]);
          double dist = (E - ball).norm();
          if (dist < 0.1 || isActive == false) {
              isActive = false;
              axisFixer();
      }


           
          //build_new_rotate(i, zAxis1, cos(phi), sin(signX * phi));
          //build_new_rotate(i, xAxis1, cos(theta), sin(theta));
      }
  }
  

  void Viewer::makeChange()
  {
      printf("1\n");
      Eigen::Vector3d b;
      vector<Eigen::Vector3d> before;
      vector<Eigen::Vector3d> new_points;
      Eigen::Vector3d target = getDistination(0);
      double di;
      double ri;
      double lambda;
      
      for (int i = 0; i < data_list.size(); i++) {
          before.push_back(getDistination(i));
      }
      before.push_back(getTipPosition(data_list.size() - 1));
      printf("2\n");
      if (maxDistance < distToTarget(0)) {
          new_points.push_back(before.at(0));
          printf("2.5\n");
          for (int i = 1; i < data_list.size(); i++) {
              di = dist_p1Tonextp1(i);
              ri = distToTarget(i);
              lambda = di / ri;
              Eigen::Vector3d tmp(0,0,0);
              tmp += lambda * target;

              tmp += (1 - lambda) * new_points.at(i - 1);

          

              new_points.push_back(tmp);
          }
          printf("3\n");
          calculateStep(before, new_points);

      }
      else {
          for (int i = 0; i < data_list.size(); i++)
          {
              new_points.push_back(getDistination(i));
          }
          printf("4\n");
          new_points.push_back(getTipPosition(data_list.size() - 1));
          b = new_points.at(0);
          float difa = distance_2Points(new_points.at(new_points.size() - 1), target);

          while (delta < difa) {
              new_points.at(new_points.size() - 1) = target;
              for (int i = data_list.size() - 1; i > 0; i--)
              {
                  di = dist_p1Tonextp1(i);
                  ri = distance_2Points(new_points.at(i + 1), new_points.at(i));
                  lambda = di / ri;
                  new_points.at(i) = (1 - lambda) * new_points.at(i + 1) + lambda * new_points.at(i);
              }
              printf("5\n");

              new_points.at(0) = b;
              new_points.push_back(target);
              for (int i = 1; i < data_list.size(); i++)
              {
                  di = dist_p1Tonextp1(i);
                  ri = distance_2Points(new_points.at(i + 1), new_points.at(i));
                  lambda = di / ri;
                  new_points.at(i + 1) = (1 - lambda) * new_points.at(i) + lambda * new_points.at(i + 1);
              }
              difa = distance_2Points(new_points.at(new_points.size() - 1), target);
              calculateStep(before, new_points);
          }
          printf("6\n");
      }
  }

  double Viewer::distance_2Points(Eigen::Vector3d p1, Eigen::Vector3d p2) {
      //euclidian distance between 2 3D points
      return sqrt(pow((p1(0) - p2(0)),2) + pow((p1(1) - p2(1)), 2) + pow((p1(2) - p2(2)), 2));
  }

  double Viewer::distToTarget(int linkInd) {
      // getting the relevant link tip position coordinates
      Vector3d linkTip = getTipPosition(linkInd);
      Vector3d sphereCenter = getDistination(0);
      return distance_2Points(linkTip, sphereCenter);
  }
  double Viewer::dist_p1Tonextp1(int p1_ind) {
      // PRE assumption p1_ind + 1 < data_list.size()
      return distance_2Points(getTipPosition(p1_ind), getTipPosition(p1_ind - 1));
  }

  Vector3d Viewer::getTipPosition(int indx)
  {
      Matrix4d Normal1 = Eigen::Matrix4d::Ones();
      if (indx > -1)
      {
          for (int j = indx; parents[j] > -1; j = parents[j])
          {
              Normal1 = data_list[parents[j]].MakeTransd() * Normal1;
          }
          return getPointInSystem(indx, Vector3d(0, 0, 1));
      }
      else
      {
          return getPointInSystem(0, Vector3d(0, 0, -1));
      }
  }


  Vector3d Viewer::getDistination(int indx)
  {
      Matrix4d Normal1 = Eigen::Matrix4d::Ones();
      if (indx > -1)
      {
          for (int j = indx; parents[j] > -1; j = parents[j])
          {
              Normal1 = data_list[parents[j]].MakeTransd() * Normal1;
          }
          return getPointInSystem(indx, Vector3d(0, 0, 0));
      }
      else
      {
          return Vector3d(0, 0, 0);
      }
  }

  Vector3d Viewer::getPointInSystem(int indx,Vector3d point)
  {
      Matrix4d scaled_mat = ParentsTrans_mat4d(indx)* data_list[indx].MakeTransScaled();
      Vector4d vec = scaled_mat * Vector4d(point(0), point(1), point(2), 1);
      return Vector3d(vec(0), vec(1), vec(2));
  }

/*
  void Viewer::build_new_rotate(int index, int axis, float cos, float sin) {		
		data_list[index].rot(axis,cos,sin);
	}
*/

  void Viewer::FabrikAlgo() {
      //printf("starting fabric algo\n");
      Eigen::Vector3d b;
      vector<Eigen::Vector3d> before;// array to save old  points position of the links( links+1 points) 
      vector<Eigen::Vector3d> new_points;// array to save new  points position of the links( links+1 points) 
      int linknum = data_list.size()-1;//because we dont need the sphere
      int pointArraySize = linknum + 1;//( links+1 points)
      //printf("before calc target\n");
      Eigen::Vector3d target = getTarget();
      //printf("after calc target\n");
      double di= 1.6;// distance between 2 points(next to each other points) pi+1 pi will be the size of cylinder
      double ri;
      double lambda;

      //printf("before calc tip\n");
      for (int i = 0; i < pointArraySize; i++) {//pushing only the point of cylinders 
          before.push_back(getTipbyindex(i+1));// i+1 because we tart i from 0, and tip function start from 1 . the index of root cylinder
      }
      //printf("after calc tip\n");
      //before.push_back(getTipbyindex(data_list.size() - 1)); not need, pushing it above, if it sphere so maybe we will need to push it later not sure
      Vector3d p1 = getTipbyindex(1);
      double dist = distance_2Points(p1,target);
      double sumOf_di = di*(pointArraySize-1);//pointArraySize its num of pi's , pointArraySize-1 its num of (pi+1-pi)
      if (dist > sumOf_di) {
          cout << "cannot reach" << endl;
          isActive = false;
          return;
          /*
          new_points.push_back(before.at(0));//pushing p0 so we cant calc pi+1 from i=1 according to alg
          for (int i = 1; i < pointArraySize - 1; i++)
          {
              Eigen::Vector3d pi = new_points.at(i - 1);
              ri = distance_2Points(target, pi);
              lambda = di / ri;
              Eigen::Vector3d  pi_plus_1(0, 0, 0);
              pi_plus_1 += lambda * target;
              pi_plus_1 += (1 - lambda) * pi;
              new_points.push_back(pi_plus_1);

          }*/
          //calcing step rotating need
          
      }

      else { //the target is reachable put the points in new points array and we will work with them
          //printf("before calc tip in newpoint\n");
          for (int i = 0; i < pointArraySize; i++)
          {
              new_points.push_back(getTipbyindex(i+1));
              //cout << i << endl;
          }
          //printf("after calc tip in newpoint\n");

          Eigen::Vector3d p1 = new_points.at(0);
          Eigen::Vector3d pn = new_points.at(new_points.size() - 1);
          b = p1;
          double difA = distance_2Points(pn, target);

          while (delta < difA) {
              new_points.at(new_points.size() - 1) = target;// pushin target to pn
              //cout << new_points.size() - 1 << endl;
              //cout << pointArraySize - 1 << endl;
              //forward reaching
              //printf("starting forward reach\n");
              int pn_mius_1_index = pointArraySize - 2;// because pointArraySize - 2 is last index(pn)
              for (int i = pn_mius_1_index; i > 0; i--)
              {
                  //printf("before calc ri\n");
                  ri = distance_2Points(new_points.at(i + 1), new_points.at(i));
                  //printf("after calc ri");
                  lambda = di / ri;
                  //printf("calc lambda is ok\n");
                  new_points.at(i) = (1 - lambda) * new_points.at(i + 1) + lambda * new_points.at(i);
                  //printf("neew point changing is ok\n");
              }
              //got to here need to check the line above
              
              //bacward reaching
              //printf("5\n");

              new_points.at(0) = b;
              //new_points.push_back(target);// not sure if this line needed according to algorithm, so i puted in comment,needed because pn=target didnot kept in new point for some reason
              //cout << new_points.size() << endl;
              //cout << pointArraySize << endl;
              for (int i = 0; i < pointArraySize-1; i++)// because we start from 0 , if we have 4 cylinder, then 5 points, then we want to run until point n-1 , aka point 4 aka index 3 (==arraypointsize-2)
              {
                  //printf("befire calc distance\n");
                  ri = distance_2Points(new_points.at(i + 1), new_points.at(i));
                  //cout << ri << endl;
                  lambda = di / ri;
                  //cout << lambda << endl;
                  new_points.at(i + 1) = (1 - lambda) * new_points.at(i) + lambda * new_points.at(i + 1);
                  //cout << i << endl;
              }
              difA = distance_2Points(new_points.at(new_points.size() - 1), target);//new_points.at(new_points.size() - 1) is Pn
              //cout << new_points.size() << endl;
              //printf("lala1\n");
              //calculateStep(before, new_points);
              transformfab(new_points);
              //need to to calc and rotate, will not stop this loop  without it, because difa will not change because we didnt change the points and moved it yet
              //printf("lala2\n");
          }
          //if difa not bigger than 0.1 so we need to stop
          isActive = false;
          axisFixer();
          //printf("lala3\n");
      }
      //printf("lala4\n");
  }
  Eigen::Vector3d Viewer::getTarget() {
      Vector4d sphere = data_list[0].MakeTransd() * Vector4d(0, 0, 0, 1);
      return Vector3d(sphere[0], sphere[1], sphere[2]);
  }
  Eigen::Vector3d Viewer::getTipbyindex(int index) {
      //index must start from 1 and above
      Eigen::Vector4d tipvec;
      int lastindex = data_list.size();// last index of last link, we have linknum+1 points == data_list.size()
      if (index == lastindex)// if its last index of last link link
      {
          tipvec = ParentsTrans_mat4d(data_list.size() - 1) * data_list[data_list.size() - 1].MakeTransd()* Eigen::Vector4d(0, 0, 0.8, 1);
      }
      else// other index of links
      {
          tipvec = ParentsTrans_mat4d(index) * data_list[index].MakeTransd() * Eigen::Vector4d(0, 0, -0.8, 1);
      }

      return Vector3d(tipvec[0], tipvec[1], tipvec[2]);
  }

  void Viewer::transformfab(vector<Eigen::Vector3d>& p)
  {
      Eigen::Vector3d z_axis = Eigen::Vector3d(0, 0, 1);
      Eigen::Vector3d y_axis = Eigen::Vector3d(0, 1, 0);
      for (auto i = 1; i < data_list.size(); i++)
      {
          //clear_rotation(i);

          Eigen::Vector3d next_z = p[i + 1] - p[i];
          next_z.normalize();
          Eigen::Vector3d r_axis = z_axis.cross(next_z);
          r_axis.normalize();
          if (r_axis.norm() > 0.0001) {
              Eigen::Vector3d x_axis = y_axis.cross(z_axis);
              Eigen::Vector3d proj = (next_z - (next_z.dot(z_axis) * z_axis));
              proj.normalize();

              double z_deg = acos(y_axis.dot(proj));
              z_deg = z_deg < -1.0 ? -1.0 : z_deg;
              z_deg = z_deg > 1.0 ? 1.0 : z_deg; //acos(clamp(Zaxis.dot(Vp), -1.0, 1.0));

              double tmpdeg = proj.dot(x_axis) > 0 ? -1 : 1;

              z_deg = z_deg * tmpdeg;

              double x_deg = acos(z_axis.dot(next_z));
              x_deg = x_deg < -1.0 ? -1.0 : x_deg;
              x_deg = x_deg > 1.0 ? 1.0 : x_deg; //acos(clamp(Zaxis.dot(Vp), -1.0, 1.0));


              //double z_deg = degrees(glm::acos(clamp(dot(y_axis, proj), -1.0f, 1.0f))) * (dot(proj, x_axis) > 0 ? -1 : 1);
              //double x_deg = degrees(glm::acos(clamp(dot(z_axis, next_z), -1.0f, 1.0f)));
              //rot
              //y_axis = Eigen::Vector3d(MyRotate(r_axis, x_deg) * Vector4d(y_axis, 0));
              z_axis = next_z;
              data_list[i].MyRotate(r_axis,x_deg, false);
              data_list[i].MyRotate(r_axis, z_deg, true);
              //shapeRotation(z_deg, -x_deg, i);
          }
      }
  }


  //end Ass3 comment


} // end namespace
} // end namespace
}
