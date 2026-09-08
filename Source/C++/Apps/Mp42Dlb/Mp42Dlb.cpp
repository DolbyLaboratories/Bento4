/*****************************************************************
|
|    AP4 - MP4 to AAC File Converter
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "Ap4.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "MP4 To Dolby Audio (AC3, E-AC-3, AC-4, Dolby TrueHD)\n"\
               " and Dolby Vision File Converter - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2008 Axiomatic Systems, LLC"
 
/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr,
            BANNER
            "\n\nusage: mp42dlb [options] <input> <output>\n"
            "  Supporting AC-3, E-AC-3, AC-4, Dolby TrueHD and Dolby \n"
            "  Vision(HEVC based and AVC based):\n"
            "  Options:\n"
            "  --track <track_id>: select one track for output.\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   SampleArray
+---------------------------------------------------------------------*/
class SampleArray {
public:
  SampleArray(AP4_Track* track) :
    m_Track(track) {
    m_SampleCount = m_Track->GetSampleCount();
    if (m_SampleCount) {
      m_ForcedSync = new bool[m_SampleCount];
      for (unsigned int i = 0; i<m_SampleCount; i++) {
        m_ForcedSync[i] = false;
      }
    }
    else {
      m_ForcedSync = NULL;
    }
  }
  virtual ~SampleArray() {
    delete[] m_ForcedSync;
  }

  virtual AP4_Cardinal GetSampleCount() {
    return m_SampleCount;
  }
  virtual AP4_Result GetSample(AP4_Ordinal index, AP4_Sample& sample) {
    AP4_Result result = m_Track->GetSample(index, sample);
    if (AP4_SUCCEEDED(result)) {
      if (m_ForcedSync[index]) {
        sample.SetSync(true);
      }
    }
    return result;
  }
  virtual AP4_Result AddSample(AP4_Sample& /*sample*/) {
    return AP4_ERROR_NOT_SUPPORTED;
  }
  virtual void ForceSync(AP4_Ordinal index) {
    if (index < m_SampleCount) {
      m_ForcedSync[index] = true;
    }
  }

protected:
  AP4_Track*   m_Track;
  AP4_Cardinal m_SampleCount;
  bool*        m_ForcedSync;
};

/*----------------------------------------------------------------------
|   CachedSampleArray
+---------------------------------------------------------------------*/
class CachedSampleArray : public SampleArray {
public:
  CachedSampleArray(AP4_Track* track) :
    SampleArray(track) {}

  virtual AP4_Cardinal GetSampleCount() {
    return m_Samples.ItemCount();
  }
  virtual AP4_Result GetSample(AP4_Ordinal index, AP4_Sample& sample) {
    if (index >= m_Samples.ItemCount()) {
      return AP4_ERROR_OUT_OF_RANGE;
    }
    else {
      sample = m_Samples[index];
      return AP4_SUCCESS;
    }
  }
  virtual AP4_Result AddSample(AP4_Sample& sample) {
    return m_Samples.Append(sample);
  }

protected:
  AP4_Array<AP4_Sample> m_Samples;
};

/*----------------------------------------------------------------------
|   TrackCursor
+---------------------------------------------------------------------*/
class TrackCursor {
public:
  TrackCursor(AP4_Track* track, SampleArray* samples);
  ~TrackCursor();

  AP4_Result    Init();
  AP4_Result    SetSampleIndex(AP4_Ordinal sample_index);

  AP4_Track*    m_Track;
  SampleArray*  m_Samples;
  AP4_Ordinal   m_SampleIndex;
  AP4_Ordinal   m_FragmentIndex;
  AP4_Sample    m_Sample;
  AP4_UI64      m_Timestamp;
  AP4_UI64      m_UnscaledTimestamp;
  bool          m_Eos;
  AP4_TfraAtom* m_Tfra;
};

/*----------------------------------------------------------------------
|   TrackCursor::TrackCursor
+---------------------------------------------------------------------*/
TrackCursor::TrackCursor(AP4_Track* track, SampleArray* samples) :
  m_Track(track),
  m_Samples(samples),
  m_SampleIndex(0),
  m_FragmentIndex(0),
  m_Timestamp(0),
  m_UnscaledTimestamp(0),
  m_Eos(false),
  m_Tfra(new AP4_TfraAtom(0))
{
}

/*----------------------------------------------------------------------
|   TrackCursor::~TrackCursor
+---------------------------------------------------------------------*/
TrackCursor::~TrackCursor()
{
  delete m_Tfra;
  delete m_Samples;
}

/*----------------------------------------------------------------------
|   TrackCursor::Init
+---------------------------------------------------------------------*/
AP4_Result
TrackCursor::Init()
{
  return m_Samples->GetSample(0, m_Sample);
}

/*----------------------------------------------------------------------
|   TrackCursor::SetSampleIndex
+---------------------------------------------------------------------*/
AP4_Result
TrackCursor::SetSampleIndex(AP4_Ordinal sample_index)
{
  m_SampleIndex = sample_index;

  // check if we're at the end
  if (sample_index >= m_Samples->GetSampleCount()) {
    AP4_UI64 end_dts = m_Sample.GetDts() + m_Sample.GetDuration();
    m_Sample.Reset();
    m_Sample.SetDts(end_dts);
    m_Eos = true;
  }
  else {
    return m_Samples->GetSample(m_SampleIndex, m_Sample);
  }

  return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   WriteAc4Header
+---------------------------------------------------------------------*/
static AP4_Result
WriteAc4Header(AP4_ByteStream* output,
               unsigned int    frame_size)
{
  unsigned char bits[7];

  bits[0] = 0xAC;
  bits[1] = 0x40;
  bits[2] = 0xFF;
  bits[3] = 0xFF;
  bits[4] = ((frame_size) & 0xFF0000) >> 16;
  bits[5] = ((frame_size) & 0x00FF00) >> 8;
  bits[6] = ((frame_size) & 0x0000FF);

  return output->Write(bits, 7);

  /*
  0-7:  syncword 12 always: 'AC40', right now only support AC40
  8-15: 0xFF
  16-55: frame_size
  */
}

/*----------------------------------------------------------------------
|   WriteH264Sample
+---------------------------------------------------------------------*/
static void
WriteH264Sample(const AP4_DataBuffer& sample_data,
            AP4_DataBuffer&       prefix,
            unsigned int          nalu_length_size,
            AP4_ByteStream*       output)
{
  const unsigned char* data = sample_data.GetData();
  unsigned int         data_size = sample_data.GetDataSize();

  // allocate a buffer for the PES packet
  AP4_DataBuffer frame_data;
  unsigned char* frame_buffer = NULL;

  // add a delimiter if we don't already have one
  bool have_access_unit_delimiter = (data_size >  nalu_length_size) && ((data[nalu_length_size] & 0x1F) == AP4_AVC_NAL_UNIT_TYPE_ACCESS_UNIT_DELIMITER);
  if (!have_access_unit_delimiter) {
    AP4_Size frame_data_size = frame_data.GetDataSize();
    frame_data.SetDataSize(frame_data_size + 6);
    frame_buffer = frame_data.UseData() + frame_data_size;

    // start of access unit
    frame_buffer[0] = 0;
    frame_buffer[1] = 0;
    frame_buffer[2] = 0;
    frame_buffer[3] = 1;
    frame_buffer[4] = 9;    // NAL type = Access Unit Delimiter;
    frame_buffer[5] = 0xE0; // Slice types = ANY
  }

  // write the NAL units
  bool prefix_added = false;
  while (data_size) {
    // sanity check
    if (data_size < nalu_length_size) break;

    // get the next NAL unit
    AP4_UI32 nalu_size;
    if (nalu_length_size == 1) {
      nalu_size = *data++;
      data_size--;
    }
    else if (nalu_length_size == 2) {
      nalu_size = AP4_BytesToInt16BE(data);
      data += 2;
      data_size -= 2;
    }
    else if (nalu_length_size == 4) {
      nalu_size = AP4_BytesToInt32BE(data);
      data += 4;
      data_size -= 4;
    }
    else {
      break;
    }
    if (nalu_size > data_size) break;

    // add the prefix if needed
    if (prefix.GetDataSize() && !prefix_added && !have_access_unit_delimiter) {
      AP4_Size frame_data_size = frame_data.GetDataSize();
      frame_data.SetDataSize(frame_data_size + prefix.GetDataSize());
      frame_buffer = frame_data.UseData() + frame_data_size;
      AP4_CopyMemory(frame_buffer, prefix.GetData(), prefix.GetDataSize());
      prefix_added = true;
    }

    // add a start code before the NAL unit
    AP4_Size frame_data_size = frame_data.GetDataSize();
    frame_data.SetDataSize(frame_data_size + 3 + nalu_size);
    frame_buffer = frame_data.UseData() + frame_data_size;
    frame_buffer[0] = 0;
    frame_buffer[1] = 0;
    frame_buffer[2] = 1;
    AP4_CopyMemory(frame_buffer + 3, data, nalu_size);

    // add the prefix if needed
    if (prefix.GetDataSize() && !prefix_added) {
      AP4_Size frame_data_size = frame_data.GetDataSize();
      frame_data.SetDataSize(frame_data_size + prefix.GetDataSize());
      frame_buffer = frame_data.UseData() + frame_data_size;
      AP4_CopyMemory(frame_buffer, prefix.GetData(), prefix.GetDataSize());
      prefix_added = true;
    }

    // move to the next NAL unit
    data += nalu_size;
    data_size -= nalu_size;
  }

  output->Write(frame_data.GetData(), frame_data.GetDataSize());
}

/*----------------------------------------------------------------------
|   MakeH264FramePrefix
+---------------------------------------------------------------------*/
static AP4_Result
MakeH264FramePrefix(AP4_SampleDescription* sdesc, AP4_DataBuffer& prefix, unsigned int& nalu_length_size)
{
  AP4_AvcSampleDescription* avc_desc = AP4_DYNAMIC_CAST(AP4_AvcSampleDescription, sdesc);
  if (avc_desc == NULL) {
    fprintf(stderr, "ERROR: track does not contain an AVC stream\n");
    return AP4_FAILURE;
  }

  nalu_length_size = avc_desc->GetNaluLengthSize();

  if (sdesc->GetFormat() == AP4_SAMPLE_FORMAT_AVC3 ||
      sdesc->GetFormat() == AP4_SAMPLE_FORMAT_AVC4 ||
      sdesc->GetFormat() == AP4_SAMPLE_FORMAT_DVAV) {
    // no need for a prefix, SPS/PPS NALs should be in the elementary stream already
    return AP4_SUCCESS;
  }

  // make the SPS/PPS prefix
  for (unsigned int i = 0; i<avc_desc->GetSequenceParameters().ItemCount(); i++) {
    AP4_DataBuffer& buffer = avc_desc->GetSequenceParameters()[i];
    unsigned int prefix_size = prefix.GetDataSize();
    prefix.SetDataSize(prefix_size + 4 + buffer.GetDataSize());
    unsigned char* p = prefix.UseData() + prefix_size;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = 1;
    AP4_CopyMemory(p, buffer.GetData(), buffer.GetDataSize());
  }
  for (unsigned int i = 0; i<avc_desc->GetPictureParameters().ItemCount(); i++) {
    AP4_DataBuffer& buffer = avc_desc->GetPictureParameters()[i];
    unsigned int prefix_size = prefix.GetDataSize();
    prefix.SetDataSize(prefix_size + 4 + buffer.GetDataSize());
    unsigned char* p = prefix.UseData() + prefix_size;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = 1;
    AP4_CopyMemory(p, buffer.GetData(), buffer.GetDataSize());
  }

  return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   WriteH265Sample
+---------------------------------------------------------------------*/
static void
WriteH265Sample(const AP4_DataBuffer& sample_data,
            AP4_DataBuffer&       prefix,
            unsigned int          nalu_length_size,
            AP4_ByteStream*       output)
{
  const unsigned char* data = sample_data.GetData();
  unsigned int         data_size = sample_data.GetDataSize();

  // detect if we have VPS/SPS/PPS and/or AUD NAL units already
  bool have_param_sets = false;
  bool have_access_unit_delimiter = false;
  while (data_size) {
    // sanity check
    if (data_size < nalu_length_size) break;

    // get the next NAL unit
    AP4_UI32 nalu_size;
    if (nalu_length_size == 1) {
      nalu_size = *data++;
      data_size--;
    }
    else if (nalu_length_size == 2) {
      nalu_size = AP4_BytesToInt16BE(data);
      data += 2;
      data_size -= 2;
    }
    else if (nalu_length_size == 4) {
      nalu_size = AP4_BytesToInt32BE(data);
      data += 4;
      data_size -= 4;
    }
    else {
      break;
    }
    if (nalu_size > data_size) break;

    unsigned int nal_unit_type = (data[0] >> 1) & 0x3F;
    if (nal_unit_type == AP4_HEVC_NALU_TYPE_AUD_NUT) {
      have_access_unit_delimiter = true;
    }
    if (nal_unit_type == AP4_HEVC_NALU_TYPE_VPS_NUT ||
        nal_unit_type == AP4_HEVC_NALU_TYPE_SPS_NUT ||
        nal_unit_type == AP4_HEVC_NALU_TYPE_PPS_NUT) {
      have_param_sets = true;
      break;
    }

    // move to the next NAL unit
    data += nalu_size;
    data_size -= nalu_size;
  }
  data = sample_data.GetData();
  data_size = sample_data.GetDataSize();

  // allocate a buffer for the frame data
  AP4_DataBuffer frame_data;
  unsigned char* frame_buffer = NULL;

  // add a delimiter if we don't already have one
  if (data_size && !have_access_unit_delimiter) {
    AP4_Size frame_data_size = frame_data.GetDataSize();
    frame_data.SetDataSize(frame_data_size + 7);
    frame_buffer = frame_data.UseData() + frame_data_size;

    // start of access unit
    frame_buffer[0] = 0;
    frame_buffer[1] = 0;
    frame_buffer[2] = 0;
    frame_buffer[3] = 1;
    frame_buffer[4] = AP4_HEVC_NALU_TYPE_AUD_NUT << 1;
    frame_buffer[5] = 1;
    frame_buffer[6] = 0x40; // pic_type = 2 (B,P,I)
  }

  // write the NAL units
  bool prefix_added = false;
  while (data_size) {
    // sanity check
    if (data_size < nalu_length_size) break;

    // get the next NAL unit
    AP4_UI32 nalu_size;
    if (nalu_length_size == 1) {
      nalu_size = *data++;
      data_size--;
    }
    else if (nalu_length_size == 2) {
      nalu_size = AP4_BytesToInt16BE(data);
      data += 2;
      data_size -= 2;
    }
    else if (nalu_length_size == 4) {
      nalu_size = AP4_BytesToInt32BE(data);
      data += 4;
      data_size -= 4;
    }
    else {
      break;
    }
    if (nalu_size > data_size) break;

    // add the prefix if needed
    if (!have_param_sets && !prefix_added && !have_access_unit_delimiter) {
      AP4_Size frame_data_size = frame_data.GetDataSize();
      frame_data.SetDataSize(frame_data_size + prefix.GetDataSize());
      frame_buffer = frame_data.UseData() + frame_data_size;
      AP4_CopyMemory(frame_buffer, prefix.GetData(), prefix.GetDataSize());
      prefix_added = true;
    }

    // add a start code before the NAL unit
    AP4_Size frame_data_size = frame_data.GetDataSize();
    frame_data.SetDataSize(frame_data_size + 3 + nalu_size);
    frame_buffer = frame_data.UseData() + frame_data_size;
    frame_buffer[0] = 0;
    frame_buffer[1] = 0;
    frame_buffer[2] = 1;
    AP4_CopyMemory(frame_buffer + 3, data, nalu_size);

    // add the prefix if needed
    if (!have_param_sets && !prefix_added) {
      frame_data_size = frame_data.GetDataSize();
      frame_data.SetDataSize(frame_data_size + prefix.GetDataSize());
      frame_buffer = frame_data.UseData() + frame_data_size;
      AP4_CopyMemory(frame_buffer, prefix.GetData(), prefix.GetDataSize());
      prefix_added = true;
    }

    // move to the next NAL unit
    data += nalu_size;
    data_size -= nalu_size;
  }

  output->Write(frame_data.GetData(), frame_data.GetDataSize());
}

/*----------------------------------------------------------------------
|   MakeH265FramePrefix
+---------------------------------------------------------------------*/
static AP4_Result
MakeH265FramePrefix(AP4_SampleDescription* sdesc, AP4_DataBuffer& prefix, unsigned int& nalu_length_size)
{
  AP4_HevcSampleDescription* hevc_desc = AP4_DYNAMIC_CAST(AP4_HevcSampleDescription, sdesc);
  if (hevc_desc == NULL) {
    fprintf(stderr, "ERROR: track does not contain an HEVC stream\n");
    return AP4_FAILURE;
  }

  // extract the nalu length size
  nalu_length_size = hevc_desc->GetNaluLengthSize();

  // make the VPS/SPS/PPS prefix
  for (unsigned int i = 0; i<hevc_desc->GetSequences().ItemCount(); i++) {
    const AP4_HvccAtom::Sequence& seq = hevc_desc->GetSequences()[i];
    if (seq.m_NaluType == AP4_HEVC_NALU_TYPE_VPS_NUT) {
      for (unsigned int j = 0; j<seq.m_Nalus.ItemCount(); j++) {
        const AP4_DataBuffer& buffer = seq.m_Nalus[j];
        unsigned int prefix_size = prefix.GetDataSize();
        prefix.SetDataSize(prefix_size + 4 + buffer.GetDataSize());
        unsigned char* p = prefix.UseData() + prefix_size;
        *p++ = 0;
        *p++ = 0;
        *p++ = 0;
        *p++ = 1;
        AP4_CopyMemory(p, buffer.GetData(), buffer.GetDataSize());
      }
    }
  }

  for (unsigned int i = 0; i<hevc_desc->GetSequences().ItemCount(); i++) {
    const AP4_HvccAtom::Sequence& seq = hevc_desc->GetSequences()[i];
    if (seq.m_NaluType == AP4_HEVC_NALU_TYPE_SPS_NUT) {
      for (unsigned int j = 0; j<seq.m_Nalus.ItemCount(); j++) {
        const AP4_DataBuffer& buffer = seq.m_Nalus[j];
        unsigned int prefix_size = prefix.GetDataSize();
        prefix.SetDataSize(prefix_size + 4 + buffer.GetDataSize());
        unsigned char* p = prefix.UseData() + prefix_size;
        *p++ = 0;
        *p++ = 0;
        *p++ = 0;
        *p++ = 1;
        AP4_CopyMemory(p, buffer.GetData(), buffer.GetDataSize());
      }
    }
  }

  for (unsigned int i = 0; i<hevc_desc->GetSequences().ItemCount(); i++) {
    const AP4_HvccAtom::Sequence& seq = hevc_desc->GetSequences()[i];
    if (seq.m_NaluType == AP4_HEVC_NALU_TYPE_PPS_NUT) {
      for (unsigned int j = 0; j<seq.m_Nalus.ItemCount(); j++) {
        const AP4_DataBuffer& buffer = seq.m_Nalus[j];
        unsigned int prefix_size = prefix.GetDataSize();
        prefix.SetDataSize(prefix_size + 4 + buffer.GetDataSize());
        unsigned char* p = prefix.UseData() + prefix_size;
        *p++ = 0;
        *p++ = 0;
        *p++ = 0;
        *p++ = 1;
        AP4_CopyMemory(p, buffer.GetData(), buffer.GetDataSize());
      }
    }
  }

  return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   WriteSamples
+---------------------------------------------------------------------*/
static void
WriteSamples(TrackCursor*           cursor,
             AP4_SampleDescription* sdesc,
             AP4_ByteStream*        output)
{
    AP4_Sample     sample;
    AP4_DataBuffer data;
    AP4_Result     result;
    cursor->Init();
    if (sdesc->GetType() == AP4_SampleDescription::TYPE_AC3 ||
        sdesc->GetType() == AP4_SampleDescription::TYPE_EAC3 ||
        sdesc->GetType() == AP4_SampleDescription::TYPE_MLP ||
        sdesc->GetType() == AP4_SampleDescription::TYPE_AC4) {
      for (unsigned int i = 0; i < cursor->m_Samples->GetSampleCount(); i++) {
        if (AP4_SUCCEEDED(cursor->m_Samples->GetSample(i, sample))) {
          if (sdesc->GetType() == AP4_SampleDescription::TYPE_AC4) {
            WriteAc4Header(output, sample.GetSize());
          }
          if (AP4_SUCCEEDED(sample.ReadData(data))) {
            output->Write(data.GetData(), data.GetDataSize());
          }
        }
        result = cursor->SetSampleIndex(cursor->m_SampleIndex + 1);
        if (AP4_FAILED(result)) {
          fprintf(stderr, "ERROR: failed to get sample %d (%d)\n", cursor->m_SampleIndex + 1, result);
          return;
        }
      }
    } else if (sdesc->GetType() == AP4_SampleDescription::TYPE_AVC) {
      // make the frame prefix
      unsigned int   nalu_length_size = 0;
      AP4_DataBuffer prefix;
      if (AP4_FAILED(MakeH264FramePrefix(sdesc, prefix, nalu_length_size))) {
        return;
      }
      for (unsigned int i = 0; i < cursor->m_Samples->GetSampleCount(); i++) {
        if (AP4_SUCCEEDED(cursor->m_Samples->GetSample(i, sample))) {
          if (AP4_SUCCEEDED(sample.ReadData(data))) {
            WriteH264Sample(data, prefix, nalu_length_size, output);
          }
        }
        result = cursor->SetSampleIndex(cursor->m_SampleIndex + 1);
        if (AP4_FAILED(result)) {
          fprintf(stderr, "ERROR: failed to get sample %d (%d)\n", cursor->m_SampleIndex + 1, result);
          return;
        }
      }
    }
    else if (sdesc->GetType() == AP4_SampleDescription::TYPE_HEVC) {
      // make the frame prefix
      unsigned int   nalu_length_size = 0;
      AP4_DataBuffer prefix;
      if (AP4_FAILED(MakeH265FramePrefix(sdesc, prefix, nalu_length_size))) {
        return;
      }
      for (unsigned int i = 0; i < cursor->m_Samples->GetSampleCount(); i++) {
        if (AP4_SUCCEEDED(cursor->m_Samples->GetSample(i, sample))) {
          if (AP4_SUCCEEDED(sample.ReadData(data))) {
            WriteH265Sample(data, prefix, nalu_length_size, output);
          }
        }
        result = cursor->SetSampleIndex(cursor->m_SampleIndex + 1);
        if (AP4_FAILED(result)) {
          fprintf(stderr, "ERROR: failed to get sample %d (%d)\n", cursor->m_SampleIndex + 1, result);
          return;
        }
      }
    }
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    int return_value = 1;
    
    if (argc < 3) {
        PrintUsageAndExit();
    }
    
    // parse command line
    AP4_Result result;
    char** args = argv+1;
    unsigned char key[16];
    bool          key_option = false;
    AP4_UI32      track_id = 1;

    while (char* arg = *++argv) {
      if (!strcmp(arg, "--help")) {
        PrintUsageAndExit();
      } else if (!strcmp(arg, "--track")) {
        track_id = atoi(*++args);
        args++;
      }
    }

    AP4_ByteStream* input        = NULL;
    AP4_File* input_file         = NULL;
    AP4_ByteStream* output       = NULL;
    AP4_Movie*      movie        = NULL;
    AP4_Track*      track        = NULL;
    SampleArray*    sample_array = NULL;
    TrackCursor*    cursor       = NULL;
    AP4_String      codec;

    // create the input stream
    result = AP4_FileByteStream::Create(*args++, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input (%d)\n", result);
        goto end;
    }
    
    // create the output stream
    result = AP4_FileByteStream::Create(*args++, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output (%d)\n", result);
        goto end;
    }

    // open the file
    input_file = new AP4_File(*input, true);

    // get the movie
    AP4_SampleDescription* sample_description;
    movie = input_file->GetMovie();
    if (movie == NULL) {
        fprintf(stderr, "ERROR: no movie in file\n");
        goto end;
    }

    track = movie->GetTrack(track_id);
    if (track == NULL) {
        fprintf(stderr, "ERROR: track %d does not exist.\n", track_id);
        goto end;
    } else if (track->GetSampleCount() == 0 && !movie->HasFragments()) {
        fprintf(stderr, "ERROR: track %d has no samples, it will be skipped\n", track->GetId());
        goto end;
    }

    // create a sample array for this track
    if (movie->HasFragments()) {
      sample_array = new CachedSampleArray(track);
    }
    else {
      sample_array = new SampleArray(track);
    }
    // create a cursor for the track
    cursor = new TrackCursor(track, sample_array);
    cursor->m_Tfra->SetTrackId(track_id);

    // remember where the stream was
    AP4_Position position;
    input->Tell(position);

    if (movie->HasFragments()) {
      AP4_LinearReader reader(*movie, input);
      reader.EnableTrack(cursor->m_Track->GetId());
//      AP4_UI32 track_id;
      AP4_Sample sample;
      do {
        result = reader.GetNextSample(sample, track_id);
        if (AP4_SUCCEEDED(result)) {
          cursor->m_Samples->AddSample(sample);
        }
      } while (AP4_SUCCEEDED(result));
    }

    // check that the track is of the right type
    sample_description = track->GetSampleDescription(0);
    if (sample_description == NULL) {
        fprintf(stderr, "ERROR: unable to parse sample description\n");
        goto end;
    }

    // show info
    if (!AP4_FAILED(sample_description->GetCodecString(codec))) {
        if (track->GetType() == AP4_Track::TYPE_VIDEO) {
            AP4_Debug("Video Track:\n");
        } else if (track->GetType() == AP4_Track::TYPE_AUDIO) {
            AP4_Debug("Audio Track:\n");
        }
        AP4_Debug("  codec: %s\n", codec.GetChars());
        AP4_Debug("  duration: %u ms\n", (int)track->GetDurationMs());
        AP4_Debug("  sample count: %u\n", cursor->m_Samples->GetSampleCount());
    }

    if (sample_description->GetType() == AP4_SampleDescription::TYPE_PROTECTED) {
        if (!key_option) {
            fprintf(stderr, "ERROR: encrypted tracks found, please use mp4decrypt first\n");
            return_value = 1;
        }
        result = 0;
    } else if (sample_description->GetType() != AP4_SampleDescription::TYPE_UNKNOWN) {
        WriteSamples(cursor, sample_description, output);
        return_value = 0;
    }

end:
    delete input_file;
    if (input) input->Release();
    if (output) output->Release();

    return return_value;
}

