#include "mfem.hpp"

using namespace mfem;

Mesh Tri2Quad(const Mesh &tri_mesh)
{
   // Verify validity
   MFEM_VERIFY(tri_mesh.Dimension() == 2, "");
   {
      Array<Geometry::Type> geoms;
      tri_mesh.GetGeometries(2, geoms);
      MFEM_VERIFY(geoms.Size() == 1, "");
      MFEM_VERIFY(geoms[0] == Geometry::TRIANGLE, "");
   }

   const int sdim = tri_mesh.SpaceDimension();

   const int nv_tri = tri_mesh.GetNV();
   const int nedge_tri = tri_mesh.GetNEdges();
   const int ntri = tri_mesh.GetNE();
   const int nbe_tri = tri_mesh.GetNBE();

   // Add new vertices in every edge and element
   const int nv = nv_tri + nedge_tri + ntri;
   const int nquad = 3*ntri; // Each tri subdivided into 3 quads
   const int nbe = 2*nbe_tri; // Each edge is subdivided into 2

   Mesh quad_mesh(2, nv, nquad, nbe, sdim);

   auto add_midpoint = [&](const Array<int> &vs)
   {
      double v[3] = {0.0, 0.0, 0.0};
      const int nvs = vs.Size();
      for (const int iv : vs)
      {
         for (int d = 0; d < 3; ++ d)
         {
            v[d] += tri_mesh.GetVertex(iv)[d] / double(nvs);
         }
      }
      quad_mesh.AddVertex(v);
   };

   // Add original vertices
   for (int iv = 0; iv < nv_tri; ++iv)
   {
      quad_mesh.AddVertex(tri_mesh.GetVertex(iv));
   }

   // Add midpoints of edges
   for (int iedge = 0; iedge < nedge_tri; ++iedge)
   {
      Array<int> edge_v;
      tri_mesh.GetEdgeVertices(iedge, edge_v);
      add_midpoint(edge_v);
   }

   // Add midpoint of triangles
   for (int itri = 0; itri < ntri; ++itri)
   {
      Array<int> tri_v;
      tri_mesh.GetElementVertices(itri, tri_v);
      add_midpoint(tri_v);
   }

   // Connectivity of tet vertices to the edges
   const int tri_vertex_edge_map[3*2] =
   {
      0, 2,
      1, 0,
      2, 1
   };

   // Add three quads for each triangle
   for (int itri = 0; itri < ntri; ++itri)
   {
      Array<int> tri_v, tri_edges, orientations;
      tri_mesh.GetElementVertices(itri, tri_v);
      tri_mesh.GetElementEdges(itri, tri_edges, orientations);

      const int attribute = tri_mesh.GetAttribute(itri);

      // One quad for each vertex of the triangle
      for (int iv = 0; iv < 3; ++iv)
      {
         const int quad_v[4] =
         {
            tri_v[iv],
            nv_tri + tri_edges[tri_vertex_edge_map[2*iv + 0]],
            nv_tri + nedge_tri + itri,
            nv_tri + tri_edges[tri_vertex_edge_map[2*iv + 1]]
         };
         quad_mesh.AddQuad(quad_v, attribute);
      }
   }

   // Add the boundary elements
   for (int ibe = 0; ibe < nbe_tri; ++ibe)
   {
      Array<int> be_v, be_edge, orientation;
      tri_mesh.GetBdrElementVertices(ibe, be_v);
      tri_mesh.GetBdrElementEdges(ibe, be_edge, orientation);

      const int attribute = tri_mesh.GetBdrAttribute(ibe);

      for (int iv = 0; iv < 2; ++iv)
      {
         int segment_v[2] = {be_v[iv], nv_tri + be_edge[0]};
         if (iv == 1) { std::reverse(segment_v, segment_v + 2); }
         quad_mesh.AddBdrQuad(segment_v, attribute);
      }
   }

   quad_mesh.FinalizeTopology();
   quad_mesh.Finalize(false, true);

   return quad_mesh;
}

int main(int argc, char *argv[])
{
   std::string mesh_file = MFEM_SOURCE_DIR "/data/beam-tri.mesh";
   std::string output_file = "";
   bool paraview = false;

   OptionsParser args(argc, argv);
   args.AddOption(&mesh_file, "-m", "--mesh", "Mesh file to use.");
   args.AddOption(&output_file, "-o", "--output", "Mesh file to write to.");
   args.AddOption(&paraview, "-pv", "--paraview", "-no-pv", "--no-paraview",
                  "ParaView (VTU) output?");
   args.ParseCheck();

   if (output_file == "")
   {
      size_t ext = mesh_file.rfind('.');
      output_file = mesh_file.substr(0, ext) + "_t2q.mesh";
   }

   Mesh tri_mesh = Mesh::LoadFromFile(mesh_file);
   Mesh quad_mesh = Tri2Quad(tri_mesh);

   quad_mesh.Save(output_file);

   if (paraview)
   {
      quad_mesh.PrintVTU(output_file);
      quad_mesh.PrintBdrVTU(output_file + "_bdr");
   }

   return 0;
}
