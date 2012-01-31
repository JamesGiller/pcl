/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2010-2011, Willow Garage, Inc.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 *
 */

#ifndef PCL_IO_PLY_IO_H_
#define PCL_IO_PLY_IO_H_

#include "pcl/io/file_io.h"
#include "pcl/io/ply/ply_parser.h"
#include <pcl/PolygonMesh.h>
#include <sstream>

namespace pcl
{
  /** \brief Point Cloud Data (PLY) file format reader.
    *
    * The PLY data format is organized in the following way:
    * lines beginning with "comment" are treated as comments
    *   - ply
    *   - format [ascii|binary_little_endian|binary_big_endian] 1.0
    *   - element vertex COUNT
    *   - property float x 
    *   - property float y 
    *   - [property float z] 
    *   - [property float normal_x] 
    *   - [property float normal_y] 
    *   - [property float normal_z] 
    *   - [property uchar red] 
    *   - [property uchar green] 
    *   - [property uchar blue] ...
    *   - ascii/binary point coordinates
    *   - [element camera 1]
    *   - [property float view_px] ...
    *   - [element range_grid COUNT]
    *   - [property list uchar int vertex_indices]
    *   - end header
    *
    * \author Nizar Sallem
    * \ingroup io
    */
  class PCL_EXPORTS PLYReader : public FileReader
  {
    public:
      enum
      {
        PLY_V0 = 0,
        PLY_V1 = 1
      };
      
      PLYReader ()
        : FileReader ()
        , origin_ (Eigen::Vector4f::Zero ())
        , orientation_ (Eigen::Matrix3f::Zero ())
        , range_grid_(0)
      {}

      ~PLYReader () { delete range_grid_; }
      /** \brief Read a point cloud data header from a PLY file.
        *
        * Load only the meta information (number of points, their types, etc),
        * and not the points themselves, from a given PLY file. Useful for fast
        * evaluation of the underlying data structure.
        *
        * Returns:
        *  * < 0 (-1) on error
        *  * > 0 on success
        * \param[in] file_name the name of the file to load
        * \param[out] cloud the resultant point cloud dataset (only the header will be filled)
        * \param[in] origin the sensor data acquisition origin (translation)
        * \param[in] orientation the sensor data acquisition origin (rotation)
        * \param[out] ply_version the PLY version read from the file
        * \param[out] data_type the type of PLY data stored in the file
        * \param[out] data_idx the data index
        */
      int 
      readHeader (const std::string &file_name, sensor_msgs::PointCloud2 &cloud,
                  Eigen::Vector4f &origin, Eigen::Quaternionf &orientation,
                  int &ply_version, int &data_type, int &data_idx);

      /** \brief Read a point cloud data from a PLY file and store it into a sensor_msgs/PointCloud2.
        * \param[in] file_name the name of the file containing the actual PointCloud data
        * \param[out] cloud the resultant PointCloud message read from disk
        * \param[in] origin the sensor data acquisition origin (translation)
        * \param[in] orientation the sensor data acquisition origin (rotation)
        * \param[out] ply_version the PLY version read from the file
        */
      int 
      read (const std::string &file_name, sensor_msgs::PointCloud2 &cloud,
            Eigen::Vector4f &origin, Eigen::Quaternionf &orientation, int& ply_version);

      /** \brief Read a point cloud data from a PLY file (PLY_V6 only!) and store it into a sensor_msgs/PointCloud2.
        *
        * \note This function is provided for backwards compatibility only and
        * it can only read PLY_V6 files correctly, as sensor_msgs::PointCloud2
        * does not contain a sensor origin/orientation. Reading any file
        * > PLY_V6 will generate a warning.
        *
        * \param[in] file_name the name of the file containing the actual PointCloud data
        * \param[out] cloud the resultant PointCloud message read from disk
        */
      inline int 
      read (const std::string &file_name, sensor_msgs::PointCloud2 &cloud)
      {
        Eigen::Vector4f origin;
        Eigen::Quaternionf orientation;
        int ply_version;
        return read (file_name, cloud, origin, orientation, ply_version);
      }

      /** \brief Read a point cloud data from any PLY file, and convert it to the given template format.
        * \param[in] file_name the name of the file containing the actual PointCloud data
        * \param[out] cloud the resultant PointCloud message read from disk
        */
      template<typename PointT> inline int
      read (const std::string &file_name, pcl::PointCloud<PointT> &cloud)
      {
        sensor_msgs::PointCloud2 blob;
        int ply_version;
        int res = read (file_name, blob, cloud.sensor_origin_, cloud.sensor_orientation_,
                        ply_version);

        // Exit in case of error
        if (res < 0)
          return (res);
        pcl::fromROSMsg (blob, cloud);
        return (0);
      }
      
    private:
      ::pcl::io::ply::ply_parser parser_;

      bool
      parse (const std::string& istream_filename);

      /** \brief Info callback function
        * \param[in] filename PLY file read
        * \param[in] line_number line triggering the callback
        * \param[in] message information message
        */
      void 
      infoCallback(const std::string& filename, std::size_t line_number, const std::string& message)
      {
        PCL_DEBUG ("[pcl::PLYReader] %s:%lu: %s\n", filename.c_str (), line_number, message.c_str ());
      }
      
      /** \brief Warning callback function
        * \param[in] filename PLY file read
        * \param[in] line_number line triggering the callback
        * \param[in] message warning message
        */
      void 
      warningCallback(const std::string& filename, std::size_t line_number, const std::string& message)
      {
        PCL_WARN ("[pcl::PLYReader] %s:%lu: %s\n", filename.c_str (), line_number, message.c_str ());
      }
      
      /** \brief Error callback function
        * \param[in] filename PLY file read
        * \param[in] line_number line triggering the callback
        * \param[in] message error message
        */
      void 
      errorCallback(const std::string& filename, std::size_t line_number, const std::string& message)
      {
        PCL_ERROR ("[pcl::PLYReader] %s:%lu: %s\n", filename.c_str (), line_number, message.c_str ());
      }
      
      /** \brief function called when the keyword element is parsed
        * \param[in] element_name element name
        * \param[in] count number of instances
        */
      std::tr1::tuple<std::tr1::function<void()>, std::tr1::function<void()> > 
      elementDefinitionCallback (const std::string& element_name, std::size_t count);
      
      bool
      endHeaderCallback ();

      /** \brief function called when a scalar property is parsed
        * \param[in] element_name element name to which the property belongs
        * \param[in] property_name property name
        */
      template <typename ScalarType> std::tr1::function<void (ScalarType)> 
      scalarPropertyDefinitionCallback (const std::string& element_name, const std::string& property_name);

      /** \brief function called when a list property is parsed
        * \param[in] element_name element name to which the property belongs
        * \param[in] property_name list property name
        */
      template <typename SizeType, typename ScalarType> 
      std::tr1::tuple<std::tr1::function<void (SizeType)>, std::tr1::function<void (ScalarType)>, std::tr1::function<void ()> > 
        listPropertyDefinitionCallback (const std::string& element_name, const std::string& property_name);

      inline void
      vertexFloatPropertyCallback (pcl::io::ply::float32 value);

      inline void
      vertexColorCallback (const std::string& color_name, pcl::io::ply::uint8 color);

      inline void
      vertexIntensityCallback (pcl::io::ply::uint8 intensity);
      
      inline void
      originXCallback (const float& value) { origin_[0] = value; }
      
      inline void
      originYCallback (const float& value) { origin_[1] = value; }
      
      inline void
      originZCallback (const float& value) { origin_[2] = value; }
    
      inline void
      orientationXaxisXCallback (const float& value) { orientation_ (0,0) = value; }
      
      inline void
      orientationXaxisYCallback (const float& value) { orientation_ (0,1) = value; }
      
      inline void
      orientationXaxisZCallback (const float& value) { orientation_ (0,2) = value; }
      
      inline void
      orientationYaxisXCallback (const float& value) { orientation_ (1,0) = value; }
      
      inline void
      orientationYaxisYCallback (const float& value) { orientation_ (1,1) = value; }

      inline void
      orientationYaxisZCallback (const float& value) { orientation_ (1,2) = value; }
      
      inline void
      orientationZaxisXCallback (const float& value) { orientation_ (2,0) = value; }
    
      inline void
      orientationZaxisYCallback (const float& value) { orientation_ (2,1) = value; }
      
      inline void
      orientationZaxisZCallback (const float& value) { orientation_ (2,2) = value; }
      
      inline void
      cloudHeightCallback (const int &height) { cloud_->height = height; }

      inline void
      cloudWidthCallback (const int &width) { cloud_->width = width; }
        
      void
      appendFloatProperty (const std::string& name, const size_t& size = 1);

      void vertexBeginCallback ();
      void vertexEndCallback ();
      void rangeGridBeginCallback ();
      void rangeGridVertexIndicesBeginCallback (pcl::io::ply::uint8 size);
      void rangeGridVertexIndicesElementCallback (pcl::io::ply::int32 vertex_index);
      void rangeGridVertexIndicesEndCallback ();
      void rangeGridEndCallback ();
      /* void faceBegin(); */
      /* void faceVertexIndicesBegin(pcl::io::ply::uint8 size); */
      /* void faceVertexIndices_element(pcl::io::ply::int32 vertex_index); */
      /* void faceVertexIndicesEnd(); */
      /* void faceEnd(); */
      void objInfoCallback (const std::string& line);
      //origin
      Eigen::Vector4f origin_;
      //orientation
      Eigen::Matrix3f orientation_;
      //vertex element artifacts
      sensor_msgs::PointCloud2 *cloud_;
      size_t vertex_count_, vertex_properties_counter_;
      int vertex_offset_before_;
      //range element artifacts
      std::vector<std::vector <int> > *range_grid_;
      size_t range_count_, range_grid_vertex_indices_element_index_;
      size_t rgb_offset_before_;
  };

  /** \brief Point Cloud Data (PLY) file format writer.
    * \author Nizar Sallem
    * \ingroup io
    */
  class PCL_EXPORTS PLYWriter : public FileWriter
  {
    public:
      PLYWriter () : mask_ (0) {};
      ~PLYWriter () {};

      /** \brief Generate the header of a PLY v.7 file format
        * \param[in] cloud the point cloud data message
        */
      inline std::string
      generateHeaderBinary (const sensor_msgs::PointCloud2 &cloud, 
                            const Eigen::Vector4f &origin, 
                            const Eigen::Quaternionf &orientation,
                            int valid_points,
                            bool use_camera = true)
      {
        return (generateHeader (cloud, origin, orientation, true, use_camera, valid_points));
      }
      
      /** \brief Generate the header of a PLY v.7 file format
        * \param[in] cloud the point cloud data message
        */
      inline std::string
      generateHeaderASCII (const sensor_msgs::PointCloud2 &cloud, 
                           const Eigen::Vector4f &origin, 
                           const Eigen::Quaternionf &orientation,
                           int valid_points,
                           bool use_camera = true)
      {
        return (generateHeader (cloud, origin, orientation, false, use_camera, valid_points));
      }

      /** \brief Save point cloud data to a PLY file containing n-D points, in ASCII format
        * \param[in] file_name the output file name
        * \param[in] cloud the point cloud data message
        * \param[in] origin the sensor data acquisition origin (translation)
        * \param[in] orientation the sensor data acquisition origin (rotation)
        * \param[in] precision the specified output numeric stream precision (default: 8)
        */
      int 
      writeASCII (const std::string &file_name, const sensor_msgs::PointCloud2 &cloud, 
                  const Eigen::Vector4f &origin = Eigen::Vector4f::Zero (), 
                  const Eigen::Quaternionf &orientation = Eigen::Quaternionf::Identity (),
                  int precision = 8,
                  bool use_camera = true);

      /** \brief Save point cloud data to a PLY file containing n-D points, in BINARY format
        * \param[in] file_name the output file name
        * \param[in] cloud the point cloud data message
        * \param[in] origin the sensor data acquisition origin (translation)
        * \param[in] orientation the sensor data acquisition origin (rotation)
        */
      int 
      writeBinary (const std::string &file_name, const sensor_msgs::PointCloud2 &cloud,
                   const Eigen::Vector4f &origin = Eigen::Vector4f::Zero (), 
                   const Eigen::Quaternionf &orientation = Eigen::Quaternionf::Identity ());

      /** \brief Save point cloud data to a PLY file containing n-D points
        * \param[in] file_name the output file name
        * \param[in] cloud the point cloud data message
        * \param[in] origin the sensor acquisition origin
        * \param[in] orientation the sensor acquisition orientation
        * \param[in] binary set to true if the file is to be written in a binary
        * PLY format, false (default) for ASCII
        */
      inline int
      write (const std::string &file_name, const sensor_msgs::PointCloud2 &cloud, 
             const Eigen::Vector4f &origin = Eigen::Vector4f::Zero (), 
             const Eigen::Quaternionf &orientation = Eigen::Quaternionf::Identity (),
             bool binary = false)
      {
        if (binary)
          return (this->writeBinary (file_name, cloud, origin, orientation));
        else
          return (this->writeASCII (file_name, cloud, origin, orientation, 8, true));
      }

      /** \brief Save point cloud data to a PLY file containing n-D points
        * \param[in] file_name the output file name
        * \param[in] cloud the point cloud data message
        * \param[in] origin the sensor acquisition origin
        * \param[in] orientation the sensor acquisition orientation
        * \param[in] binary set to true if the file is to be written in a binary
        * \param[in] use_camera set to true to used camera element and false to
        * use range_grid element
        * PLY format, false (default) for ASCII
        */
      inline int
      write (const std::string &file_name, const sensor_msgs::PointCloud2 &cloud, 
             const Eigen::Vector4f &origin = Eigen::Vector4f::Zero (), 
             const Eigen::Quaternionf &orientation = Eigen::Quaternionf::Identity (),
             bool binary = false,
             bool use_camera = true)
      {
        if (binary)
          return (this->writeBinary (file_name, cloud, origin, orientation));
        else
          return (this->writeASCII (file_name, cloud, origin, orientation, 8, use_camera));
      }

      /** \brief Save point cloud data to a PLY file containing n-D points
        * \param[in] file_name the output file name
        * \param[in] cloud the point cloud data message (boost shared pointer)
        * \param[in] origin the sensor acquisition origin
        * \param[in] orientation the sensor acquisition orientation
        * \param[in] binary set to true if the file is to be written in a binary
        * PLY format, false (default) for ASCII
        */
      inline int
      write (const std::string &file_name, const sensor_msgs::PointCloud2::ConstPtr &cloud, 
             const Eigen::Vector4f &origin = Eigen::Vector4f::Zero (), 
             const Eigen::Quaternionf &orientation = Eigen::Quaternionf::Identity (),
             bool binary = false,
             bool use_camera = true)
      {
        return (write (file_name, *cloud, origin, orientation, binary, use_camera));
      }

      /** \brief Save point cloud data to a PLY file containing n-D points
        * \param[in] file_name the output file name
        * \param[in] cloud the pcl::PointCloud data
        * \param[in] binary set to true if the file is to be written in a binary
        * PLY format, false (default) for ASCII
        */
      template<typename PointT> inline int
      write (const std::string &file_name, 
             const pcl::PointCloud<PointT> &cloud, 
             bool binary = false,
             bool use_camera = true)
      {
        Eigen::Vector4f origin = cloud.sensor_origin_;
        Eigen::Quaternionf orientation = cloud.sensor_orientation_;

        sensor_msgs::PointCloud2 blob;
        pcl::toROSMsg (cloud, blob);

        // Save the data
        return (this->write (file_name, blob, origin, orientation, binary, use_camera));
      }
      
    private:
      /** \brief Generate a PLY header.
        * \param[in] cloud the input point cloud
        * \param[in] binary whether the PLY file should be saved as binary data (true) or ascii (false)
        */
      std::string
      generateHeader (const sensor_msgs::PointCloud2 &cloud, 
                      const Eigen::Vector4f &origin, 
                      const Eigen::Quaternionf &orientation,
                      bool binary, 
                      bool use_camera,
                      int valid_points);

      void
      writeContentWithCameraASCII (int nr_points, 
                                   int point_size,
                                   const sensor_msgs::PointCloud2 &cloud, 
                                   const Eigen::Vector4f &origin, 
                                   const Eigen::Quaternionf &orientation,
                                   std::ofstream& fs);

      void
      writeContentWithRangeGridASCII (int nr_points, 
                                      int point_size,
                                      const sensor_msgs::PointCloud2 &cloud, 
                                      std::ostringstream& fs,
                                      int& nb_valid_points);

      /** \brief Construct a mask from a list of fields.
        * \param[in] fields_list the list of fields to construct a mask from
        */
      void 
      setMaskFromFieldsList (const std::string& fields_list);

      /** \brief Internally used mask. */
      int mask_;
  };

  namespace io
  {
    /** \brief Load a PLY v.6 file into a templated PointCloud type.
      *
      * Any PLY files containg sensor data will generate a warning as a
      * sensor_msgs/PointCloud2 message cannot hold the sensor origin.
      *
      * \param[in] file_name the name of the file to load
      * \param[in] cloud the resultant templated point cloud
      * \ingroup io
      */
    inline int
    loadPLYFile (const std::string &file_name, sensor_msgs::PointCloud2 &cloud)
    {
      pcl::PLYReader p;
      return (p.read (file_name, cloud));
    }

    /** \brief Load any PLY file into a templated PointCloud type.
      * \param[in] file_name the name of the file to load
      * \param[in] cloud the resultant templated point cloud
      * \param[in] origin the sensor acquisition origin (only for > PLY_V7 - null if not present)
      * \param[in] orientation the sensor acquisition orientation if availble, 
      * identity if not present
      * \ingroup io
      */
    inline int
    loadPLYFile (const std::string &file_name, sensor_msgs::PointCloud2 &cloud,
                 Eigen::Vector4f &origin, Eigen::Quaternionf &orientation)
    {
      pcl::PLYReader p;
      int ply_version;
      return (p.read (file_name, cloud, origin, orientation, ply_version));
    }

    /** \brief Load any PLY file into a templated PointCloud type
      * \param[in] file_name the name of the file to load
      * \param[in] cloud the resultant templated point cloud
      * \ingroup io
      */
    template<typename PointT> inline int
    loadPLYFile (const std::string &file_name, pcl::PointCloud<PointT> &cloud)
    {
      pcl::PLYReader p;
      return (p.read (file_name, cloud));
    }

    /** \brief Save point cloud data to a PLY file containing n-D points
      * \param[in] file_name the output file name
      * \param[in] cloud the point cloud data message
      * \param[in] origin the sensor data acquisition origin (translation)
      * \param[in] orientation the sensor data acquisition origin (rotation)
      * \param[in] binary_mode true for binary mode, false (default) for ASCII
      * \ingroup io
      */
    inline int 
    savePLYFile (const std::string &file_name, const sensor_msgs::PointCloud2 &cloud, 
                 const Eigen::Vector4f &origin = Eigen::Vector4f::Zero (), 
                 const Eigen::Quaternionf &orientation = Eigen::Quaternionf::Identity (),
                 bool binary_mode = false, bool use_camera = true)
    {
      PLYWriter w;
      return (w.write (file_name, cloud, origin, orientation, binary_mode, use_camera));
    }

    /** \brief Templated version for saving point cloud data to a PLY file
      * containing a specific given cloud format
      * \param[in] file_name the output file name
      * \param[in] cloud the point cloud data message
      * \param[in] binary_mode true for binary mode, false (default) for ASCII
      * \ingroup io
      */
    template<typename PointT> inline int
    savePLYFile (const std::string &file_name, const pcl::PointCloud<PointT> &cloud, bool binary_mode = false)
    {
      PLYWriter w;
      return (w.write<PointT> (file_name, cloud, binary_mode));
    }

    /** \brief Templated version for saving point cloud data to a PLY file
      * containing a specific given cloud format.
      * \param[in] file_name the output file name
      * \param[in] cloud the point cloud data message
      * \ingroup io
      */
    template<typename PointT> inline int
    savePLYFileASCII (const std::string &file_name, const pcl::PointCloud<PointT> &cloud)
    {
      PLYWriter w;
      return (w.write<PointT> (file_name, cloud, false));
    }

    /** \brief Templated version for saving point cloud data to a PLY file containing a specific given cloud format.
      * \param[in] file_name the output file name
      * \param[in] cloud the point cloud data message
      * \ingroup io
      */
    template<typename PointT> inline int
    savePLYFileBinary (const std::string &file_name, const pcl::PointCloud<PointT> &cloud)
    {
      PLYWriter w;
      return (w.write<PointT> (file_name, cloud, true));
    }

    /** \brief Templated version for saving point cloud data to a PLY file containing a specific given cloud format
      * \param[in] file_name the output file name
      * \param[in] cloud the point cloud data message
      * \param[in] indices the set of indices to save
      * \param[in] binary_mode true for binary mode, false (default) for ASCII
      * \ingroup io
      */
    template<typename PointT> int
    savePLYFile (const std::string &file_name, const pcl::PointCloud<PointT> &cloud,
                 const std::vector<int> &indices, bool binary_mode = false)
    {
      // Copy indices to a new point cloud
      pcl::PointCloud<PointT> cloud_out;
      copyPointCloud (cloud, indices, cloud_out);
      // Save the data
      PLYWriter w;
      return (w.write<PointT> (file_name, cloud_out, binary_mode));
    }

    /** \brief Saves a PolygonMesh in ascii PLY format.
      * \param[in] file_name the name of the file to write to disk
      * \param[in] mesh the polygonal mesh to save
      * \param[in] precision the output ASCII precision default 5
      * \ingroup io
      */
    PCL_EXPORTS int
    savePLYFile (const std::string &file_name, const pcl::PolygonMesh &mesh, unsigned precision = 5);

  };
}

#endif  //#ifndef PCL_IO_PLY_IO_H_
