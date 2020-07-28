﻿/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) YEAR AUTHOR_NAME AUTHOR_EMAIL
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-plugin
 *
 * FIXME:Describe plugin here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! plugin ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */


#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/gstbuffer.h>
#include <gst/gstcaps.h>

#include <opencv2/opencv.hpp>

#include "gstzedodoverlay.h"
#include "gst-zed-meta/gstzedmeta.h"

GST_DEBUG_CATEGORY_STATIC (gst_zedodoverlay_debug);
#define GST_CAT_DEFAULT gst_zedodoverlay_debug

/* Filter signals and args */
enum
{
    /* FILL ME */
    LAST_SIGNAL
};

enum
{
    PROP_0
};



/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
                                                                    GST_PAD_SINK,
                                                                    GST_PAD_ALWAYS,
                                                                    GST_STATIC_CAPS( ("video/x-raw, "
                                                                                      "format = (string) { BGRA }, "
                                                                                      "width = (int) { 672, 1280, 1920, 2208 } , "
                                                                                      "height =  (int) { 376, 720, 1080, 1242 } , "
                                                                                      "framerate =  (fraction) { 15, 30, 60, 100 }") ) );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
                                                                   GST_PAD_SRC,
                                                                   GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS( ("video/x-raw, "
                                                                                     "format = (string) { BGRA }, "
                                                                                     "width = (int) { 672, 1280, 1920, 2208 } , "
                                                                                     "height =  (int) { 376, 720, 1080, 1242 } , "
                                                                                     "framerate =  (fraction)  { 15, 30, 60, 100 }") ) );

/* class initialization */
G_DEFINE_TYPE(GstZedOdOverlay, gst_zedodoverlay, GST_TYPE_ELEMENT);

static void gst_zedodoverlay_set_property (GObject * object, guint prop_id,
                                           const GValue * value, GParamSpec * pspec);
static void gst_zedodoverlay_get_property (GObject * object, guint prop_id,
                                           GValue * value, GParamSpec * pspec);

static gboolean gst_zedodoverlay_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_zedodoverlay_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */

/* initialize the plugin's class */
static void
gst_zedodoverlay_class_init (GstZedOdOverlayClass * klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

    GST_DEBUG_OBJECT( gobject_class, "Class Init" );

    gobject_class->set_property = gst_zedodoverlay_set_property;
    gobject_class->get_property = gst_zedodoverlay_get_property;

    gst_element_class_set_static_metadata (gstelement_class,
                                           "ZED Object Detection Overlay",
                                           "Overlay/Video",
                                           "Stereolabs ZED Object Detection Overlay",
                                           "Stereolabs <support@stereolabs.com>");

    gst_element_class_add_pad_template (gstelement_class,
                                        gst_static_pad_template_get (&src_factory));

    gst_element_class_add_pad_template (gstelement_class,
                                        gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void gst_zedodoverlay_init (GstZedOdOverlay *filter)
{
    GST_DEBUG_OBJECT( filter, "Filter Init" );

    filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
    gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

    filter->srcpad = gst_pad_new_from_static_template( &src_factory, "src" );
    gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

    gst_pad_set_event_function( filter->sinkpad, GST_DEBUG_FUNCPTR(gst_zedodoverlay_sink_event) );
    gst_pad_set_chain_function( filter->sinkpad, GST_DEBUG_FUNCPTR(gst_zedodoverlay_chain) );

    filter->caps = nullptr;

    filter->img_left_w = 0;
    filter->img_left_h = 0;
}

static void
gst_zedodoverlay_set_property (GObject * object, guint prop_id,
                               const GValue * value, GParamSpec * pspec)
{
    GstZedOdOverlay *filter = GST_ZEDODOVERLAY (object);

    GST_DEBUG_OBJECT( filter, "Set property" );

    switch (prop_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gst_zedodoverlay_get_property (GObject * object, guint prop_id,
                               GValue * value, GParamSpec * pspec)
{
    GstZedOdOverlay *filter = GST_ZEDODOVERLAY (object);

    GST_DEBUG_OBJECT( filter, "Get property" );

    switch (prop_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

/* GstElement vmethod implementations */

static gboolean set_out_caps( GstZedOdOverlay* filter, GstCaps* sink_caps )
{
    GstVideoInfo vinfo_in;
    GstVideoInfo vinfo_left;

    GST_DEBUG_OBJECT( filter, "Sink caps %" GST_PTR_FORMAT, sink_caps);

    // ----> Caps source
    if (filter->caps) {
        gst_caps_unref (filter->caps);
    }

    gst_video_info_from_caps(&vinfo_in, sink_caps);

    filter->img_left_w = vinfo_in.width;
    filter->img_left_h = vinfo_in.height;

    gst_video_info_init( &vinfo_left );
    gst_video_info_set_format( &vinfo_left, GST_VIDEO_FORMAT_BGRA,
                               vinfo_in.width, vinfo_in.height );
    vinfo_left.fps_d = vinfo_in.fps_d;
    vinfo_left.fps_n = vinfo_in.fps_n;
    filter->caps = gst_video_info_to_caps(&vinfo_left);

    GST_DEBUG_OBJECT( filter, "Created left caps %" GST_PTR_FORMAT, filter->caps );
    if(gst_pad_set_caps(filter->srcpad, filter->caps)==FALSE)
    {
        return false;
    }
    // <---- Caps source


    return TRUE;
}

/* this function handles sink events */
static gboolean gst_zedodoverlay_sink_event(GstPad* pad, GstObject* parent, GstEvent* event)
{
    GstZedOdOverlay *filter;
    gboolean ret;

    filter = GST_ZEDODOVERLAY (parent);

    GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
                    GST_EVENT_TYPE_NAME (event), event);

    switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
        GST_DEBUG_OBJECT( filter, "Event CAPS" );
        GstCaps* caps;

        gst_event_parse_caps(event, &caps);

        ret = set_out_caps( filter, caps );

        /* and forward */
        ret = gst_pad_event_default (pad, parent, event);
        break;
    }
    default:
        ret = gst_pad_event_default (pad, parent, event);
        break;
    }

    return ret;
}

void draw_objects( cv::Mat& image, guint8 obj_count, ZedObjectData* objs, gfloat scaleW, gfloat scaleH )
{
    for( int i=0; i<obj_count; i++ )
    {
        cv::Scalar color = cv::Scalar::all(125);
        if(objs[i].id>=0)
        {
            color = cv::Scalar((objs[i].id*232+232)%255, (objs[i].id*176+176)%255, (objs[i].id*59+59)%255);
        }

        cv::Rect roi_render(0, 0, image.size().width, image.size().height);

        GST_TRACE( "Object: %d", i );
        GST_TRACE( " * Id: %d", (int)objs[i].label );
        GST_TRACE( " * Pos: %g,%g,%g", objs[i].position[0],objs[i].position[1],objs[i].position[2] );

        if(objs[i].skeletons_avail==FALSE)
        {
            GST_TRACE( "Scale: %g, %g",scaleW,scaleH );

            // ----> Bounding box

            cv::Point2f tl;
            tl.x = objs[i].bounding_box_2d[0][0]*scaleW;
            tl.y = objs[i].bounding_box_2d[0][1]*scaleH;
            cv::Point2f br;
            br.x = objs[i].bounding_box_2d[2][0]*scaleW;
            br.y = objs[i].bounding_box_2d[2][1]*scaleH;
            cv::rectangle( image, tl, br, color, 3 );

            // <---- Bounding box

            // ----> Text info
            int baseline=0;
            int font_face = cv::FONT_HERSHEY_COMPLEX_SMALL;
            double font_scale = 0.75;

            std::stringstream txt_info;
            txt_info << "Id: " << objs[i].id << " - " << ((objs[i].label==OBJECT_CLASS::PERSON)?"PERSON":"VEHICLE");

            cv::Size txt_size = cv::getTextSize( txt_info.str(), font_face, font_scale, 1, &baseline );

            int offset = 5;

            cv::Point txt_tl;
            cv::Point txt_br;
            cv::Point txt_pos;

            txt_tl.x = tl.x;
            txt_tl.y = tl.y - (txt_size.height+2*offset);

            txt_br.x = tl.x + (txt_size.width+2*offset);
            txt_br.y = tl.y;

            txt_pos.x = txt_tl.x + offset;
            txt_pos.y = txt_br.y - offset;

            if( !roi_render.contains(txt_tl) )
            {
                txt_tl.y = tl.y + (txt_size.height+2*baseline);
                txt_pos.y = txt_tl.y - offset;
            }

            cv::rectangle( image, txt_tl, txt_br, color, 1 );
            cv::putText( image, txt_info.str(), txt_pos, font_face, font_scale, color, 1, cv::LINE_AA );

            if( !std::isnan(objs[i].position[0]) && !std::isnan(objs[i].position[1]) && !std::isnan(objs[i].position[2]) )
            {
                float dist = sqrtf( objs[i].position[0]*objs[i].position[0] +
                        objs[i].position[1]*objs[i].position[1] +
                        objs[i].position[2]*objs[i].position[2]);
                char text[64];
                sprintf(text, "%.2fm", abs(dist / 1000.0f));
                putText( image, text, cv::Point2i(tl.x+(br.x-tl.x)/2 - 20, tl.y+(br.y-tl.y)/2 - 12),
                         cv::FONT_HERSHEY_COMPLEX_SMALL, 0.75, color, 1 );
            }
            // <---- Text info
        }
        else
        {
            GST_TRACE( "Scale: %g, %g",scaleW,scaleH );
            // ----> Skeletons
            {
                // ----> Bones
                for (const auto& parts : skeleton::BODY_BONES)
                {
                    if( objs[i].keypoint_2d[skeleton::getIdx(parts.first)][0]>=0 &&
                            objs[i].keypoint_2d[skeleton::getIdx(parts.first)][1]>=0 &&
                            objs[i].keypoint_2d[skeleton::getIdx(parts.second)][0]>=0 &&
                            objs[i].keypoint_2d[skeleton::getIdx(parts.second)][1]>=0 )
                    {
                        cv::Point2f kp_a;
                        kp_a.x = objs[i].keypoint_2d[skeleton::getIdx(parts.first)][0]*scaleW;
                        kp_a.y = objs[i].keypoint_2d[skeleton::getIdx(parts.first)][1]*scaleH;
                        GST_TRACE( "kp_a: %g, %g",kp_a.x,kp_a.y );

                        cv::Point2f kp_b;
                        kp_b.x = objs[i].keypoint_2d[skeleton::getIdx(parts.second)][0]*scaleW;
                        kp_b.y = objs[i].keypoint_2d[skeleton::getIdx(parts.second)][1]*scaleH;
                        GST_TRACE( "kp_b: %g, %g",kp_b.x,kp_b.y );

                        if (roi_render.contains(kp_a) && roi_render.contains(kp_b))
                            cv::line(image, kp_a, kp_b, color, 1, cv::LINE_AA);
                    }
                }
                // <---- Bones
                // ----> Joints
                for(int j=0; j<18; j++)
                {
                    if( objs[i].keypoint_2d[j][0]>=0 &&
                            objs[i].keypoint_2d[j][1]>=0 )
                    {
                        cv::Point2f cv_kp;
                        cv_kp.x = objs[i].keypoint_2d[j][0]*scaleW;
                        cv_kp.y = objs[i].keypoint_2d[j][1]*scaleH;
                        if (roi_render.contains(cv_kp))
                        {
                            cv::circle(image, cv_kp, 3, color+cv::Scalar(50,50,50), -1, cv::LINE_AA);
                        }
                    }
                }
                // <---- Joints
            }
            // <---- Skeletons
        }
    }
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn gst_zedodoverlay_chain(GstPad* pad, GstObject * parent, GstBuffer* buf )
{
    GstZedOdOverlay *filter;

    filter = GST_ZEDODOVERLAY (parent);

    GST_TRACE_OBJECT( filter, "Chain" );

    GstMapInfo map_in;
    GstMapInfo map_out;

    GstZedSrcMeta* meta = nullptr;

    GstFlowReturn ret = GST_FLOW_ERROR;

    GstClockTime timestamp = GST_CLOCK_TIME_NONE;
    timestamp = GST_BUFFER_TIMESTAMP (buf);
    GST_LOG ("timestamp %" GST_TIME_FORMAT, GST_TIME_ARGS (timestamp));

    GST_TRACE_OBJECT( filter, "Processing ..." );
    if(gst_buffer_map(buf, &map_in, (GstMapFlags)(GST_MAP_WRITE|GST_MAP_READ)))
    {
        GST_TRACE ("Input buffer size %lu B", map_in.size );

//        GST_TRACE ("Output buffer allocation - size %lu B", map_in.size );
//        GstBuffer* proc_buf = gst_buffer_new_allocate(NULL, map_in.size, NULL );

//        if( !GST_IS_BUFFER(proc_buf) )
//        {
//            GST_DEBUG ("Out buffer not allocated");

//            // ----> Release incoming buffer
//            gst_buffer_unmap( buf, &map_in );
//            gst_buffer_unref(buf);
//            // <---- Release incoming buffer

//            return GST_FLOW_ERROR;
//        }

        //if( gst_buffer_map(proc_buf, &map_out, (GstMapFlags)(GST_MAP_WRITE)) )
        //{
            //GST_TRACE ("Copying buffer %lu B", map_out.size );
            //memcpy(map_out.data, map_in.data, map_out.size);

            // Get image
            //cv::Mat frame = cv::Mat( filter->img_left_h, filter->img_left_w, CV_8UC4, map_out.data );
            cv::Mat frame = cv::Mat( filter->img_left_h, filter->img_left_w, CV_8UC4, map_in.data );

            // Get metadata
            meta = (GstZedSrcMeta*)gst_buffer_get_meta( buf, GST_ZED_SRC_META_API_TYPE );

            if(meta==NULL)
            {
                GST_WARNING( "The ZED Stream does not contain ZED metadata" );
            }

            if( meta==NULL ) // Metadata not found
            {
                GST_ELEMENT_ERROR (filter, RESOURCE, FAILED,
                                   ("No ZED metadata [GstZedSrcMeta] found in the stream. No overlay'" ), (NULL));

                gst_buffer_unmap( buf, &map_in );
                gst_buffer_unref(buf);

                return GST_FLOW_ERROR;
            }

            GST_TRACE_OBJECT( filter, "Cam. Model: %d",meta->info.cam_model );
            GST_TRACE_OBJECT( filter, "Stream type: %d",meta->info.stream_type );
            GST_TRACE_OBJECT( filter, "Grab frame Size: %d x %d",meta->info.grab_frame_width,meta->info.grab_frame_height );

            gboolean rescaled = FALSE;
            gfloat scaleW = 1.0f;
            gfloat scaleH = 1.0f;
            if(meta->info.grab_frame_width != filter->img_left_w ||
                    meta->info.grab_frame_height != filter->img_left_h)
            {
                rescaled = TRUE;
                scaleW = ((gfloat)filter->img_left_w)/meta->info.grab_frame_width;
                scaleH = ((gfloat)filter->img_left_h)/meta->info.grab_frame_height;
            }

            if(meta->od_enabled)
            {
                GST_TRACE_OBJECT( filter, "Detected %d objects",meta->obj_count );
                // Draw 2D detections
                draw_objects( frame, meta->obj_count, meta->objects, scaleW, scaleH );
            }

            //GST_TRACE ("Out buffer set timestamp" );
            //GST_BUFFER_PTS(proc_buf) = GST_BUFFER_PTS (buf);
            //GST_BUFFER_DTS(proc_buf) = GST_BUFFER_DTS (buf);
            //GST_BUFFER_TIMESTAMP(proc_buf) = GST_BUFFER_TIMESTAMP (buf);

            GST_TRACE ("Out buffer push" );
            //ret = gst_pad_push(filter->srcpad, proc_buf);
            ret = gst_pad_push(filter->srcpad, buf);

            if( ret != GST_FLOW_OK )
            {
                GST_DEBUG_OBJECT( filter, "Error pushing out buffer: %s", gst_flow_get_name (ret));

                // ----> Release incoming buffer
                gst_buffer_unmap( buf, &map_in );
                gst_buffer_unref(buf);
//                GST_TRACE ("Out buffer unmap" );
//                gst_buffer_unmap(proc_buf, &map_out);
//                gst_buffer_unref(proc_buf);
                // <---- Release incoming buffer
                return ret;
            }

            //GST_TRACE ("Out buffer unmap" );
            //gst_buffer_unmap(proc_buf, &map_out);
            //gst_buffer_unref(proc_buf);
//        }
//        else
//        {
//            GST_ELEMENT_ERROR (pad, RESOURCE, FAILED,
//                               ("Failed to map buffer for writing" ), (NULL));
//            return GST_FLOW_ERROR;
//        }

        // ----> Release incoming buffer.
        //GST_TRACE ("In buffer unmap" );
        //gst_buffer_unmap( buf, &map_in );
        //gst_buffer_unref(buf); // NOTE: required to not increase memory consumption exponentially
        // <---- Release incoming buffer
    }
    else
    {
        GST_ELEMENT_ERROR (pad, RESOURCE, FAILED,
                           ("Failed to map buffer for reading" ), (NULL));
        return GST_FLOW_ERROR;
    }
    GST_TRACE ("... processed" );

    if(ret==GST_FLOW_OK)
    {
        GST_TRACE_OBJECT( filter, "Chain OK" );
        GST_LOG( "**************************" );
        return GST_FLOW_OK;
    }

    return GST_FLOW_ERROR;
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean plugin_init (GstPlugin * plugin)
{
    /* debug category for fltering log messages
   *
   * exchange the string 'Template plugin' with your description
   */
    GST_DEBUG_CATEGORY_INIT( gst_zedodoverlay_debug, "zedodoverlay",
                             0, "debug category for zedodoverlay element");

    gst_element_register( plugin, "zedodoverlay", GST_RANK_NONE,
                          gst_zedodoverlay_get_type());

    return TRUE;
}

/* gstreamer looks for this structure to register plugins
 *
 * exchange the string 'Template plugin' with your plugin description
 */
GST_PLUGIN_DEFINE( GST_VERSION_MAJOR,
                   GST_VERSION_MINOR,
                   zedodoverlay,
                   "ZED OBject Detection Overlay",
                   plugin_init,
                   GST_PACKAGE_VERSION,
                   GST_PACKAGE_LICENSE,
                   GST_PACKAGE_NAME,
                   GST_PACKAGE_ORIGIN)
