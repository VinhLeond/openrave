// Copyright (C) 2010 Rosen Diankov
//
// boundingmeshpy is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#ifndef OPENRAVE_DISABLE_ASSERT_HANDLER
#define BOOST_ENABLE_ASSERT_HANDLER
#endif
#include <iostream>
#define PY_ARRAY_UNIQUE_SYMBOL PyArrayHandle
#include <boost/python.hpp>
#include <boost/python/exception_translator.hpp>
#include <boost/python/stl_iterator.hpp>
#include <pyconfig.h>
#include <numpy/arrayobject.h>

#include <exception>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/assert.hpp>

#define OPENRAVE_BININGS_PYARRAY
#include "bindings.h"

#include <openrave/openrave.h>
#include "openravepy_int.h"
#include "boundingmesh.h"

using namespace boost::python;
using namespace std;
using namespace openravepy;
using OpenRAVE::TriMesh;
using OpenRAVE::Vector;

class PyTriMesh;

object _DecimatorCallbackFunction;

// TODO : check the name of the last two parameters
void _DecimatorCallback(unsigned int vertex, boundingmesh::Real error) {
  PyGILState_STATE gstate = PyGILState_Ensure();
  try {
    _DecimatorCallbackFunction(vertex, error);
  }
  catch(...) {
    RAVELOG_ERROR("exception occured in python decimator callback.\n");
    PyErr_Print();
  }
  PyGILState_Release(gstate);
}

void ExtractMesh(const TriMesh& openraveMesh, boundingmesh::Mesh& mesh) {
  int numverts = openraveMesh.vertices.size();
  for(int i = 0; i < numverts; ++i) {
    Vector v = openraveMesh.vertices[i];
    mesh.addVertex(boundingmesh::Vector3(v[0],v[1],v[2]));
  }

  int numtris = openraveMesh.indices.size();
  BOOST_ASSERT( numtris % 3 == 0 );
  for(int i = 0; i < numtris / 3; ++i) {
    mesh.addTriangle(openraveMesh.indices[3*i + 0], openraveMesh.indices[3*i + 1], openraveMesh.indices[3*i + 2]);
  }
}

void ExtractTriMesh(const boundingmesh::Mesh& mesh, TriMesh& openraveMesh) {
  int numverts = mesh.nVertices();
  openraveMesh.vertices.resize(numverts);
  for(int i = 0 ; i < numverts; ++i) {
    openraveMesh.vertices[i] = Vector(mesh.vertex(i).position()(0), mesh.vertex(i).position()(1), mesh.vertex(i).position()(2));
  }

  int numtris = mesh.nTriangles();
  openraveMesh.indices.resize(3*numtris);
  for(int i = 0; i < numtris; ++i) {
    openraveMesh.indices[3*i + 0] = mesh.triangle(i).vertex(0);
    openraveMesh.indices[3*i + 1] = mesh.triangle(i).vertex(1);
    openraveMesh.indices[3*i + 2] = mesh.triangle(i).vertex(2);
  }
}

object LoadFromFile(str pyfilename) {
  const std::string filename((const char*)(extract<const char*>(pyfilename)));
  boundingmesh::Mesh mesh;
  if( pyfilename.endswith(".obj") ) {
    mesh.loadObj(filename);
  } else if ( pyfilename.endswith(".off") ) {
    mesh.loadOff(filename);
  } else if( pyfilename.endswith(".wrl") ) {
    mesh.loadWrl(filename);
  } else if( pyfilename.endswith(".stl") ) {
    mesh.loadStl(filename);
  } else {
    return object();
  }
  TriMesh openraveMesh;
  ExtractTriMesh(mesh, openraveMesh);
  return toPyTriMesh(openraveMesh);
}


// note : boundingmesh::Real == double
/// \param oMesh python object containing a PyTriMesh
/// \param direction the direction in which the bounding mesh shoul grow
/// \param targetVertices the number of vertices to reach (default : 1000)
/// \param maximumError the maximum error accepted for the approximation (default : 1.0)
/// \param metric
/// \param initialization
/// \param fncallback function void(int, float) called by the decimator during decimation
/// \return an python object containing a PyTriMesh
object ComputeBoundingMesh(object oMesh,
                           int targetVertices = boundingmesh::default_target_vertices,
                           boundingmesh::Real maximumError = boundingmesh::default_maximum_error,
                           boundingmesh::DecimationDirection direction = boundingmesh::Outward,
                           boundingmesh::Metric metric = boundingmesh::ClassicQEM,
                           boundingmesh::Initialization initialization = boundingmesh::Midpoint,
                           object fncallback = object()) {
  TriMesh openraveMesh;

  if( !ExtractTriMesh(oMesh, openraveMesh) ) {
    return object();
  }

  // create and load the mesh
  boundingmesh::Mesh mesh;
  ExtractMesh(openraveMesh, mesh);

  // create the decimator and set the options
  boundingmesh::Decimator decimator;
  decimator.setDirection(direction);
  decimator.setMetric(metric);
  decimator.setInitialization(initialization);
  decimator.setTargetVertices(targetVertices);
  decimator.setMaximumError(maximumError);
  decimator.setMesh(mesh);

  // Do the decimation
  // TODO : the decimator accepts a callback
  //boundingmesh::Mesh* result_mesh = &mesh;
  std::shared_ptr<boundingmesh::Mesh> result_mesh;
  if( fncallback.is_none() ) {
     result_mesh = decimator.compute();
  } else {
    _DecimatorCallbackFunction = fncallback;
    result_mesh = decimator.compute(&_DecimatorCallback);
    _DecimatorCallbackFunction = object();
  }

  // convert the result back to python
  if( result_mesh->isDirty() ) {
    result_mesh->cleanAndRenumber();
  }
  ExtractTriMesh(*result_mesh, openraveMesh);
  return toPyTriMesh(openraveMesh);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(ComputeBoundingMesh_overloads, ComputeBoundingMesh, 1, 7)

BOOST_PYTHON_MODULE(boundingmeshpy)
{
  import_array();
  numeric::array::set_module_and_type("numpy", "ndarray");
  int_from_int();

  enum_< boundingmesh::DecimationDirection >("DecimationDirection")
    .value("Outward", boundingmesh::Outward)
    .value("Inward", boundingmesh::Inward)
    .value("Any", boundingmesh::Any)
    ;

  enum_< boundingmesh::Metric >("Metric")
    .value("ClassicQEM", boundingmesh::ClassicQEM)
    .value("ModifiedQEM", boundingmesh::ModifiedQEM)
    .value("MinimizedConstant", boundingmesh::MinimizedConstant)
    .value("Diagonalization", boundingmesh::Diagonalization)
    .value("Average", boundingmesh::Average)
    ;

  enum_< boundingmesh::Initialization >("Initialization")
    .value("DistancePrimitives", boundingmesh::DistancePrimitives)
    .value("Midpoint", boundingmesh::Midpoint)
    ;

  // does this have any use ?
  typedef return_value_policy< copy_const_reference > return_copy_const_ref;

  def("LoadFromFile", LoadFromFile);

  def("ComputeBoundingMesh", ComputeBoundingMesh,
      ComputeBoundingMesh_overloads(args("mesh",
                                         "targetVerticesCount",
                                         "maximumError",
                                         "direction",
                                         "metric",
                                         "initialization",
                                         "callback"),
                                    "Andre Gaschler & Quirin Fischer's boundingmesh algorithm"));

  scope().attr("__author__") = "Andre Gaschler, Quirin Fischer";
  scope().attr("__license__") = "2-clause BSD";
}
