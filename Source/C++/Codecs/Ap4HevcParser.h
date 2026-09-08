/*****************************************************************
|
|    AP4 - HEVC Parser
|
|    Copyright 2002-2014 Axiomatic Systems, LLC
|
|
|    This file is part of Bento4/AP4 (MP4 Atom Processing Library).
|
|    Unless you have obtained Bento4 under a difference license,
|    this version of Bento4 is Bento4|GPL.
|    Bento4|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Bento4|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Bento4|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
****************************************************************/

#ifndef _AP4_HEVC_PARSER_H_
#define _AP4_HEVC_PARSER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4Results.h"
#include "Ap4DataBuffer.h"
#include "Ap4NalParser.h"
#include "Ap4Array.h"
#include "Ap4Utils.h"
#include <cstdint> 
/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int AP4_HEVC_NALU_TYPE_TRAIL_N        = 0;
const unsigned int AP4_HEVC_NALU_TYPE_TRAIL_R        = 1;
const unsigned int AP4_HEVC_NALU_TYPE_TSA_N          = 2;
const unsigned int AP4_HEVC_NALU_TYPE_TSA_R          = 3;
const unsigned int AP4_HEVC_NALU_TYPE_STSA_N         = 4;
const unsigned int AP4_HEVC_NALU_TYPE_STSA_R         = 5;
const unsigned int AP4_HEVC_NALU_TYPE_RADL_N         = 6;
const unsigned int AP4_HEVC_NALU_TYPE_RADL_R         = 7;
const unsigned int AP4_HEVC_NALU_TYPE_RASL_N         = 8;
const unsigned int AP4_HEVC_NALU_TYPE_RASL_R         = 9;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_VCL_N10    = 10;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_VCL_R11    = 11;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_VCL_N12    = 12;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_VCL_R13    = 13;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_VCL_N14    = 14;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_VCL_R15    = 15;
const unsigned int AP4_HEVC_NALU_TYPE_BLA_W_LP       = 16;
const unsigned int AP4_HEVC_NALU_TYPE_BLA_W_RADL     = 17;
const unsigned int AP4_HEVC_NALU_TYPE_BLA_N_LP       = 18;
const unsigned int AP4_HEVC_NALU_TYPE_IDR_W_RADL     = 19;
const unsigned int AP4_HEVC_NALU_TYPE_IDR_N_LP       = 20;
const unsigned int AP4_HEVC_NALU_TYPE_CRA_NUT        = 21;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_IRAP_VCL22 = 22;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_IRAP_VCL23 = 23;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_VCL24      = 24;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_VCL25      = 25;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_VCL26      = 26;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_VCL27      = 27;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_VCL28      = 28;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_VCL29      = 29;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_VCL30      = 30;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_VCL31      = 31;
const unsigned int AP4_HEVC_NALU_TYPE_VPS_NUT        = 32;
const unsigned int AP4_HEVC_NALU_TYPE_SPS_NUT        = 33;
const unsigned int AP4_HEVC_NALU_TYPE_PPS_NUT        = 34;
const unsigned int AP4_HEVC_NALU_TYPE_AUD_NUT        = 35;
const unsigned int AP4_HEVC_NALU_TYPE_EOS_NUT        = 36;
const unsigned int AP4_HEVC_NALU_TYPE_EOB_NUT        = 37;
const unsigned int AP4_HEVC_NALU_TYPE_FD_NUT         = 38;
const unsigned int AP4_HEVC_NALU_TYPE_PREFIX_SEI_NUT = 39;
const unsigned int AP4_HEVC_NALU_TYPE_SUFFIX_SEI_NUT = 40;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_NVCL41     = 41;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_NVCL42     = 42;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_NVCL43     = 43;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_NVCL44     = 44;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_NVCL45     = 45;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_NVCL46     = 46;
const unsigned int AP4_HEVC_NALU_TYPE_RSV_NVCL47     = 47;
const unsigned int AP4_HEVC_NALU_TYPE_UNSPEC62       = 62;
const unsigned int AP4_HEVC_NALU_TYPE_UNSPEC63       = 63;

const unsigned int AP4_HEVC_PPS_MAX_ID               = 63;
const unsigned int AP4_HEVC_SPS_MAX_ID               = 15;
const unsigned int AP4_HEVC_VPS_MAX_ID               = 15;
const unsigned int AP4_HEVC_SEI_MAX_TYPE             = 202;
const unsigned int AP4_HEVC_SPS_MAX_RPS              = 64;
const unsigned int AP4_HEVC_MAX_LT_REFS              = 32;

const unsigned int AP4_HEVC_ACCESS_UNIT_FLAG_IS_IDR              = 0x01;
const unsigned int AP4_HEVC_ACCESS_UNIT_FLAG_IS_IRAP             = 0x02;
const unsigned int AP4_HEVC_ACCESS_UNIT_FLAG_IS_BLA              = 0x04;
const unsigned int AP4_HEVC_ACCESS_UNIT_FLAG_IS_RADL             = 0x08;
const unsigned int AP4_HEVC_ACCESS_UNIT_FLAG_IS_RASL             = 0x10;
const unsigned int AP4_HEVC_ACCESS_UNIT_FLAG_IS_SUBLAYER_NON_REF = 0x20;

const unsigned int AP4_HEVC_SLICE_TYPE_B = 0;
const unsigned int AP4_HEVC_SLICE_TYPE_P = 1;
const unsigned int AP4_HEVC_SLICE_TYPE_I = 2;


typedef enum {
    SEI_BUFFERING_PERIOD = 0,
    SEI_PICTURE_TIMING = 1,
    SEI_PAN_SCAN_RECT = 2,
    SEI_FILLER_PAYLOAD = 3,
    SEI_USER_DATA_REGISTERED_ITU_T_T35 = 4,
    SEI_USER_DATA_UNREGISTERED = 5,
    SEI_RECOVERY_POINT = 6,
    SEI_SCENE_INFO = 9,
    SEI_FULL_FRAME_SNAPSHOT = 15,
    SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START = 16,
    SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END = 17,
    SEI_FILM_GRAIN_CHARACTERISTICS = 19,
    SEI_POST_FILTER_HINT = 22,
    SEI_TONE_MAPPING_INFO = 23,
    SEI_FRAME_PACKING = 45,
    SEI_DISPLAY_ORIENTATION = 47,
    SEI_SOP_DESCRIPTION = 128,
    SEI_ACTIVE_PARAMETER_SETS = 129,
    SEI_DECODING_UNIT_INFO = 130,
    SEI_TEMPORAL_LEVEL0_INDEX = 131,
    SEI_DECODED_PICTURE_HASH = 132,
    SEI_SCALABLE_NESTING = 133,
    SEI_REGION_REFRESH_INFO = 134,
    SEI_MASTERING_DISPLAY_COLOR_VOLUME = 137,
    SEI_LIGHT_LEVEL_INFORMATION = 144,
    SEI_AMBIENT_VIEWING_ENVIRONMENT = 148,
    SEI_3D_REFERENCE_DISPLAY = 176
} AP4_SEI_PayloadType;

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
struct AP4_HevcSliceSegmentHeader;

/*----------------------------------------------------------------------
|   AP4_HevcProfileTierLevel
+---------------------------------------------------------------------*/
struct AP4_HevcProfileTierLevel {
    AP4_HevcProfileTierLevel();
    
    // methods
    AP4_Result Parse(AP4_BitReader& bits, unsigned int max_num_sub_layers_minus_1);

    unsigned int general_profile_space;
    unsigned int general_tier_flag;
    unsigned int general_profile_idc;
    AP4_UI32     general_profile_compatibility_flags;
    
    // this is a synthetic field that includes 48 bits starting with:
    // general_progressive_source_flag    (1 bit)
    // general_interlaced_source_flag     (1 bit)
    // general_non_packed_constraint_flag (1 bit)
    // general_frame_only_constraint_flag (1 bit)
    // general_reserved_zero_44bits       (44 bits)
    AP4_UI64     general_constraint_indicator_flags;

    unsigned int general_level_idc;
    struct {
        unsigned char sub_layer_profile_present_flag;
        unsigned char sub_layer_level_present_flag;
        unsigned char sub_layer_profile_space;
        unsigned char sub_layer_tier_flag;
        unsigned char sub_layer_profile_idc;
        AP4_UI32      sub_layer_profile_compatibility_flags;
        unsigned char sub_layer_progressive_source_flag;
        unsigned char sub_layer_interlaced_source_flag;
        unsigned char sub_layer_non_packed_constraint_flag;
        unsigned char sub_layer_frame_only_constraint_flag;
        unsigned char sub_layer_level_idc;
    } sub_layer_info[8];
};

/*----------------------------------------------------------------------
|   AP4_SampleAspectRatio
+---------------------------------------------------------------------*/
struct AP4_SampleAspectRatio{
    AP4_SampleAspectRatio(unsigned int w, unsigned int h) :
        horizontal_size(w),
        vertical_size(h){}
    
    unsigned int horizontal_size;
    unsigned int vertical_size;
};

/*----------------------------------------------------------------------
|   AP4_HevcVuiParameters
+---------------------------------------------------------------------*/
struct AP4_HevcVuiParameters {
  AP4_HevcVuiParameters();

  // methods
  AP4_Result Parse(AP4_BitReader& bits, unsigned int &transfer_characteristics, unsigned int sps_max_sub_layers_minus1);
  void hrd_parameters(AP4_BitReader& bits, bool commonInfPresentFlag, uint16_t maxNumSubLayersMinus1);
  void sub_layer_hrd_parameters(AP4_BitReader& bits, uint32_t cpb_cnt, uint8_t sub_pic_hrd_params_present_flag);
  AP4_SampleAspectRatio GetSampleAspectRatio();

  unsigned int aspect_ratio_info_present_flag;
  unsigned int aspect_ratio_idc;
  unsigned int sar_width;
  unsigned int sar_height;
  unsigned int overscan_info_present_flag;
  unsigned int overscan_appropriate_flag;
  unsigned int video_signal_type_present_flag;
  unsigned int video_format;
  unsigned int video_full_range_flag;
  unsigned int colour_description_present_flag;
  unsigned int colour_primaries;
  unsigned int transfer_characteristics;
  unsigned int matrix_coeffs;
  unsigned int chroma_loc_info_present_flag;
  unsigned int chroma_sample_loc_type_top_field;
  unsigned int chroma_sample_loc_type_bottom_field;
  unsigned int neutral_chroma_indication_flag;
  unsigned int field_seq_flag;
  unsigned int frame_field_info_present_flag;
  unsigned int default_display_window_flag;
  unsigned int def_disp_win_left_offset;
  unsigned int def_disp_win_right_offset;
  unsigned int def_disp_win_top_offset;
  unsigned int def_disp_win_bottom_offset;
  unsigned int vui_timing_info_present_flag;
  unsigned int vui_num_units_in_tick;
  unsigned int vui_time_scale;
  unsigned int vui_poc_proportional_to_timing_flag;
  unsigned int vui_num_ticks_poc_diff_one_minus1;
  unsigned int vui_hrd_parameters_present_flag;

  //// skip hrd_parameters
  unsigned int bitstream_restriction_flag;
  unsigned int tiles_fixed_structure_flag;
  unsigned int motion_vectors_over_pic_boundaries_flag;
  unsigned int restricted_ref_pic_lists_flag;
  unsigned int min_spatial_segmentation_idc;
  //unsigned int max_bytes_per_pic_denom;
  //unsigned int max_bits_per_min_cu_denom;
  //unsigned int log2_max_mv_length_horizontal;
  //unsigned int log2_max_mv_length_vertical;
};


/*----------------------------------------------------------------------
|   AP4_HevcShortTermRefPicSet
+---------------------------------------------------------------------*/
typedef struct {
    unsigned int delta_poc_s0_minus1[16];
    unsigned int delta_poc_s1_minus1[16];
    unsigned int used_by_curr_pic_s0_flag[16];
    unsigned int used_by_curr_pic_s1_flag[16];
    unsigned int num_negative_pics;
    unsigned int num_positive_pics;
    unsigned int num_delta_pocs;
} AP4_HevcShortTermRefPicSet;

/*----------------------------------------------------------------------
|   AP4_HevcPictureParameterSet
+---------------------------------------------------------------------*/
struct AP4_HevcPictureParameterSet {
    AP4_HevcPictureParameterSet();
    
    // methods
    AP4_Result Parse(const unsigned char* data, unsigned int data_size);

    AP4_DataBuffer raw_bytes;
    unsigned int   pps_pic_parameter_set_id;
    unsigned int   pps_seq_parameter_set_id;
    unsigned int   dependent_slice_segments_enabled_flag;
    unsigned int   output_flag_present_flag;
    unsigned int   num_extra_slice_header_bits;
    unsigned int   sign_data_hiding_enabled_flag;
    unsigned int   cabac_init_present_flag;
    unsigned int   num_ref_idx_l0_default_active_minus1;
    unsigned int   num_ref_idx_l1_default_active_minus1;
    int            init_qp_minus26;
    unsigned int   constrained_intra_pred_flag;
    unsigned int   transform_skip_enabled_flag;
    unsigned int   cu_qp_delta_enabled_flag;
    unsigned int   diff_cu_qp_delta_depth;
    int            pps_cb_qp_offset;
    int            pps_cr_qp_offset;
    unsigned int   pps_slice_chroma_qp_offsets_present_flag;
    unsigned int   weighted_pred_flag;
    unsigned int   weighted_bipred_flag;
    unsigned int   transquant_bypass_enabled_flag;
    unsigned int   tiles_enabled_flag;
    unsigned int   entropy_coding_sync_enabled_flag;
    unsigned int   num_tile_columns_minus1;
    unsigned int   num_tile_rows_minus1;
    unsigned int   uniform_spacing_flag;
    unsigned int   loop_filter_across_tiles_enabled_flag;
    unsigned int   pps_loop_filter_across_slices_enabled_flag;
    unsigned int   deblocking_filter_control_present_flag;
    unsigned int   deblocking_filter_override_enabled_flag;
    unsigned int   pps_deblocking_filter_disabled_flag;
    int            pps_beta_offset_div2;
    int            pps_tc_offset_div2;
    unsigned int   pps_scaling_list_data_present_flag;
    unsigned int   lists_modification_present_flag;
    unsigned int   log2_parallel_merge_level_minus2;
    unsigned int   slice_segment_header_extension_present_flag;
};

/*----------------------------------------------------------------------
|   AP4_HevcSequenceParameterSet
+---------------------------------------------------------------------*/
struct AP4_HevcSequenceParameterSet {
    AP4_HevcSequenceParameterSet();
    
    // methods
    AP4_Result Parse(const unsigned char* data, unsigned int data_size);
    void GetInfo(unsigned int& width, unsigned int& height);
    void GetTimeScaleInfo(unsigned int& time_scale, unsigned int& num_units);

    AP4_DataBuffer           raw_bytes;
    unsigned int             sps_video_parameter_set_id;
    unsigned int             sps_max_sub_layers_minus1;
    unsigned int             sps_temporal_id_nesting_flag;
    AP4_HevcProfileTierLevel profile_tier_level;
    unsigned int             sps_seq_parameter_set_id;
    unsigned int             chroma_format_idc;
    unsigned int             separate_colour_plane_flag;
    unsigned int             pic_width_in_luma_samples;
    unsigned int             pic_height_in_luma_samples;
    unsigned int             conformance_window_flag;
    unsigned int             conf_win_left_offset;
    unsigned int             conf_win_right_offset;
    unsigned int             conf_win_top_offset;
    unsigned int             conf_win_bottom_offset;
    unsigned int             bit_depth_luma_minus8;
    unsigned int             bit_depth_chroma_minus8;
    unsigned int             sps_max_dec_pic_buffering_minus1[8];
    unsigned int             sps_max_num_reorder_pics[8];
    unsigned int             sps_max_latency_increase_plus1[8];
    unsigned int             log2_max_pic_order_cnt_lsb_minus4;
    unsigned int             sps_sub_layer_ordering_info_present_flag;
    unsigned int             log2_min_luma_coding_block_size_minus3;
    unsigned int             log2_diff_max_min_luma_coding_block_size;
    unsigned int             log2_min_transform_block_size_minus2;
    unsigned int             log2_diff_max_min_transform_block_size;
    unsigned int             max_transform_hierarchy_depth_inter;
    unsigned int             max_transform_hierarchy_depth_intra;
    unsigned int             scaling_list_enabled_flag;
    unsigned int             sps_scaling_list_data_present_flag;
                             // skipped scaling list data
    unsigned int             amp_enabled_flag;
    unsigned int             sample_adaptive_offset_enabled_flag;
    unsigned int             pcm_enabled_flag;
    unsigned int             pcm_sample_bit_depth_luma_minus1;
    unsigned int             pcm_sample_bit_depth_chroma_minus1;
    unsigned int             log2_min_pcm_luma_coding_block_size_minus3;
    unsigned int             log2_diff_max_min_pcm_luma_coding_block_size;
    unsigned int             pcm_loop_filter_disabled_flag;
    unsigned int             num_short_term_ref_pic_sets;
    unsigned int             long_term_ref_pics_present_flag;
    unsigned int             num_long_term_ref_pics_sps;
    unsigned int             sps_temporal_mvp_enabled_flag;
    unsigned int             strong_intra_smoothing_enabled_flag;
    unsigned int             vui_parameters_present_flag;
    AP4_HevcVuiParameters    vui_parameters;
    AP4_HevcShortTermRefPicSet short_term_ref_pic_sets[AP4_HEVC_SPS_MAX_RPS];
};

/*----------------------------------------------------------------------
|   AP4_HevcVideoParameterSet
+---------------------------------------------------------------------*/
struct AP4_HevcVideoParameterSet {
    AP4_HevcVideoParameterSet();
    
    // methods
    AP4_Result Parse(const unsigned char* data, unsigned int data_size);
    void GetTimeScaleInfo(unsigned int& time_scale, unsigned int& num_units);

    AP4_DataBuffer           raw_bytes;
    unsigned int             vps_video_parameter_set_id;
    unsigned int             vps_max_layers_minus1;
    unsigned int             vps_max_sub_layers_minus1;
    unsigned int             vps_temporal_id_nesting_flag;
    AP4_HevcProfileTierLevel profile_tier_level;
    unsigned int             vps_sub_layer_ordering_info_present_flag;
    unsigned int             vps_max_dec_pic_buffering_minus1[8];
    unsigned int             vps_max_num_reorder_pics[8];
    unsigned int             vps_max_latency_increase_plus1[8];
    unsigned int             vps_max_layer_id;
    unsigned int             vps_num_layer_sets_minus1;
    unsigned int             vps_timing_info_present_flag;
    unsigned int             vps_num_units_in_tick;
    unsigned int             vps_time_scale;
    unsigned int             vps_poc_proportional_to_timing_flag;
    unsigned int             vps_num_ticks_poc_diff_one_minus1;
};

/*----------------------------------------------------------------------
|   AP4_HevcSEIMessage
+---------------------------------------------------------------------*/
struct AP4_HevcSEIMessage {
    AP4_HevcSEIMessage();
    AP4_HevcSEIMessage(AP4_SEI_PayloadType payload_type);
    AP4_HevcSEIMessage(AP4_SEI_PayloadType payload_type, AP4_Array<AP4_UI32> payloads);

    // methods
    AP4_Result Parse(const unsigned char* data, unsigned int data_size);
    //void GetInfo(unsigned int& time_scale, unsigned int& num_units);

    AP4_DataBuffer           raw_bytes;
    AP4_SEI_PayloadType      payload_type;
    unsigned int             payload_size;

    typedef union {
        /** payloadType === 137 */
        /** HRD10 compatiable */
        /** Mastering display color volume */
        struct {
            AP4_UI16               display_primaries_x[3];    /* indicating the Mastering primary GBR x*/
            AP4_UI16               display_primaries_y[3];    /* indicating the Mastering primary GBR y*/
            AP4_UI16               white_point_x;    /* indicating the Mastering White point primary x*/
            AP4_UI16               white_point_y;    /* indicating the Mastering White point primary y*/
            AP4_UI32               max_display_mastering_luminance;    /* indicating the Mastering Luminance Max*/
            AP4_UI32               min_display_mastering_luminance;    /* indicating the Mastering Luminance Min*/
        }mdcv;
        
        /** payloadType === 148 */
        /** Ambient Viewing Environment */
        struct {
            AP4_UI32               ambient_illuminance;
            AP4_UI16               ambient_light_x;
            AP4_UI16               ambient_light_y;
        }amve;

        /** payloadType === 149 */
        /** Content light level */
        struct {
            AP4_UI32               max_content_light_level;
            AP4_UI32               max_pic_average_light_level;
        }clli;
        struct {
            AP4_UI16   uuid_iso_iec_11578;
            AP4_UI08* payload;
            AP4_UI16  payload_size;
        }udus;
    }SeiPayload;

    SeiPayload sei_payload;
};

/*----------------------------------------------------------------------
|   AP4_HevcSliceSegmentHeader
+---------------------------------------------------------------------*/
struct AP4_HevcSliceSegmentHeader {
    AP4_HevcSliceSegmentHeader() {} // leave members uninitialized on purpose
    
    AP4_Result Parse(const AP4_UI08*                data,
                     unsigned int                   data_size,
                     unsigned int                   nal_unit_type,
                     AP4_HevcPictureParameterSet**  picture_parameter_sets,
                     AP4_HevcSequenceParameterSet** sequence_parameter_sets);

    unsigned int size; // size of the parsed data
    
    unsigned int first_slice_segment_in_pic_flag;
    unsigned int no_output_of_prior_pics_flag;
    unsigned int slice_pic_parameter_set_id;
    unsigned int dependent_slice_segment_flag;
    unsigned int slice_segment_address;
    unsigned int slice_type;
    unsigned int pic_output_flag;
    unsigned int colour_plane_id;
    unsigned int slice_pic_order_cnt_lsb;
    unsigned int short_term_ref_pic_set_sps_flag;
    unsigned int short_term_ref_pic_set_idx;
    unsigned int num_entry_point_offsets;
    unsigned int offset_len_minus1;
    unsigned int num_long_term_sps;
    unsigned int num_long_term_pics;

    AP4_HevcShortTermRefPicSet short_term_ref_pic_set;
    unsigned int               used_by_curr_pic_lt_flag[AP4_HEVC_MAX_LT_REFS];
};

/*----------------------------------------------------------------------
|   AP4_HevcNalParser
+---------------------------------------------------------------------*/
class AP4_HevcNalParser : public AP4_NalParser {
public:
    static const char* NaluTypeName(unsigned int nalu_type);
    static const char* PicTypeName(unsigned int primary_pic_type);
    static const char* SliceTypeName(unsigned int slice_type);
    
    AP4_HevcNalParser();
};

/*----------------------------------------------------------------------
|   AP4_HevcFrameParser
+---------------------------------------------------------------------*/
class AP4_HevcFrameParser {
public:
    // types
    struct AccessUnitInfo {
        AP4_Array<AP4_DataBuffer*> nal_units;
        bool                       is_random_access;
        AP4_UI32                   decode_order;
        AP4_UI32                   display_order;
        
        void Reset();
    };
    
    // methods
    AP4_HevcFrameParser();
   ~AP4_HevcFrameParser();
    
    /**
     * Feed some data to the parser and look for the next NAL Unit.
     *
     * @param data Pointer to the memory buffer with the data to feed.
     * @param data_size Size in bytes of the buffer pointed to by the
     * data pointer.
     * @param bytes_consumed Number of bytes from the data buffer that were
     * consumed and stored by the parser.
     * @param access_unit_info Reference to a AccessUnitInfo structure that will
     * contain information about any access unit found in the data. If no
     * access unit was found, the nal_units field of this structure will be an
     * empty array.
     * @param eos Boolean flag that indicates if this buffer is the last
     * buffer in the stream/file (End Of Stream).
     *
     * @result: AP4_SUCCESS is the call succeeds, or an error code if it
     * fails.
     * 
     * The caller must not feed the same data twice. When this method
     * returns, the caller should inspect the value of bytes_consumed and
     * advance the input stream source accordingly, such that the next
     * buffer passed to this method will be exactly bytes_consumed bytes
     * after what was passed in this call. After all the input data has
     * been supplied to the parser (eos=true), the caller also should
     * this method with an empty buffer (data=NULL and/or data_size=0) until
     * no more data is returned, because there may be buffered data still
     * available.
     *
     * When data is returned in the access_unit_info structure, the caller is
     * responsible for freeing this data by calling AccessUnitInfo::Reset()
     */
    AP4_Result Feed(const void*     data,
                    AP4_Size        data_size,
                    AP4_Size&       bytes_consumed,
                    AccessUnitInfo& access_unit_info,
                    bool            eos=false);
    
    AP4_Result Feed(const AP4_UI08* nal_unit,
                    AP4_Size        nal_unit_size,
                    AccessUnitInfo& access_unit_info,
                    bool            last_unit=false);

    AP4_Result ParseSliceSegmentHeader(const AP4_UI08*             data,
                                       unsigned int                data_size,
                                       unsigned int                nal_unit_type,
                                       AP4_HevcSliceSegmentHeader& slice_header);
    
    bool checkIfUserSEISame(AP4_HevcSEIMessage* sei, AP4_HevcSEIMessage* pre_sei);

    AP4_HevcVideoParameterSet**    GetVideoParameterSets()    { return &m_VPS[0]; }
    AP4_HevcSequenceParameterSet** GetSequenceParameterSets() { return &m_SPS[0]; }
    AP4_HevcPictureParameterSet**  GetPictureParameterSets()  { return &m_PPS[0]; }
    AP4_HevcSEIMessage*            GetSeiMessage(AP4_SEI_PayloadType payload_type);

    void SetParameterControl(bool isKeep) { m_keepParameterSets = isKeep; }

    unsigned int GetMaxTemporalId() { return m_MaxTemporalId;}
private:
    // methods
    void CheckIfAccessUnitIsCompleted(AccessUnitInfo& access_unit_info);
    void AppendNalUnitData(const unsigned char* data, unsigned int data_size);
    
    // members
    AP4_HevcNalParser             m_NalParser;
    AP4_HevcSliceSegmentHeader*   m_CurrentSlice;
    unsigned int                  m_CurrentNalUnitType;
    unsigned int                  m_CurrentTemporalId;
    unsigned int                  m_MaxTemporalId;
    AP4_HevcPictureParameterSet*  m_PPS[AP4_HEVC_PPS_MAX_ID+1];
    AP4_HevcSequenceParameterSet* m_SPS[AP4_HEVC_SPS_MAX_ID+1];
    AP4_HevcVideoParameterSet*    m_VPS[AP4_HEVC_VPS_MAX_ID+1];
    AP4_HevcSEIMessage*           m_SEI[AP4_HEVC_SEI_MAX_TYPE+1];

    // accumulator for NAL unit data
    unsigned int               m_TotalNalUnitCount;
    unsigned int               m_TotalAccessUnitCount;
    AP4_Array<AP4_DataBuffer*> m_AccessUnitData;
    AP4_UI32                   m_AccessUnitFlags;
    unsigned int               m_VclNalUnitsInAccessUnit;
    unsigned int               m_TotalSeiCount;
    
    // picture order counting
    unsigned int               m_PrevTid0Pic_PicOrderCntMsb;
    unsigned int               m_PrevTid0Pic_PicOrderCntLsb;

    // control if the parameter sets(VPS, SPS, PPS) need to be stored in stream('mdat')
    bool                       m_keepParameterSets;
    bool                       m_keepUserSei;
};

#endif // _AP4_HEVC_PARSER_H_
