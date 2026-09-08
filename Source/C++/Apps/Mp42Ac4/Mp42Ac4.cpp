/*****************************************************************
|
|    AP4 - MP4 to AC4 File Converter
|
|    Copyright 2024 Dolby Laboratories
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
#define BANNER "MP4 To AC4 File Converter - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2024 Dolby Laboratories"
 
/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: mp42ac4 [options] <input> <output>\n"
            "  Options:\n"
            "  --key <hex>: 128-bit decryption key (in hex: 32 chars)\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   Encode framesize into either 2 or 5 bytes and return number of bytes
+---------------------------------------------------------------------*/
static int WriteFrameSize(unsigned char *bits, unsigned int size)
{
    if (size < 0xffffUL) {
        bits[1] = size & 0xff;
        size >>= 8;
        bits[0] = size & 0xff;
        return 2;
    } else {
        bits[0] = 0xff; bits[1] = 0xff;
        bits[4] = size & 0xff;
        size >>= 8;
        bits[3] = size & 0xff;
        size >>= 8;
        bits[2] = size & 0xff;
        return 5;
    }
}

/*----------------------------------------------------------------------
|   WriteSyncHeader, no CRC
+---------------------------------------------------------------------*/
static AP4_Result
WriteSyncHeader(AP4_ByteStream* output, 
                unsigned int    frame_size)
{
	unsigned char bits[7]; // 2 bytes syncword, max 5 bytes length info
    int len;

	bits[0] = 0xAC;
	bits[1] = 0x40; // no CRC
    len = 2+WriteFrameSize(bits+2, frame_size);

	return output->Write(bits, len);
}

/*----------------------------------------------------------------------
|   DecryptAndWriteSamples
+---------------------------------------------------------------------*/
static void
DecryptAndWriteSamples(AP4_Track*             track, 
                       AP4_SampleDescription* sdesc, 
                       AP4_Byte*              key, 
                       AP4_ByteStream*        output)
{
    AP4_ProtectedSampleDescription* pdesc = AP4_DYNAMIC_CAST(AP4_ProtectedSampleDescription, sdesc);
    if (pdesc == NULL) {
        fprintf(stderr, "ERROR: unable to obtain cipher info\n");
        return;
    }
    
    AP4_AudioSampleDescription* audio_desc = AP4_DYNAMIC_CAST(AP4_AudioSampleDescription, pdesc->GetOriginalSampleDescription());
    if (audio_desc == NULL) {
        fprintf(stderr, "ERROR: sample description is not audio\n");
        return;
    }
    
    // create the decrypter
    AP4_SampleDecrypter* decrypter = AP4_SampleDecrypter::Create(pdesc, key, 16);
    if (decrypter == NULL) {
        fprintf(stderr, "ERROR: unable to create decrypter\n");
        return;
    }

    AP4_Sample     sample;
    AP4_DataBuffer encrypted_data;
    AP4_DataBuffer decrypted_data;
    AP4_Ordinal    index = 0;
    while (AP4_SUCCEEDED(track->ReadSample(index, sample, encrypted_data))) {
        if (AP4_FAILED(decrypter->DecryptSampleData(encrypted_data, decrypted_data))) {
            fprintf(stderr, "ERROR: failed to decrypt sample\n");
            return;
        }

	    WriteSyncHeader(output, decrypted_data.GetDataSize());
        output->Write(decrypted_data.GetData(), decrypted_data.GetDataSize());
	    index++;
    }
}

/*----------------------------------------------------------------------
|   WriteSamples
+---------------------------------------------------------------------*/
static void
WriteSamples(AP4_Track*             track, 
             AP4_SampleDescription* sdesc,
             AP4_ByteStream*        output)
{
    AP4_AudioSampleDescription* audio_desc = AP4_DYNAMIC_CAST(AP4_AudioSampleDescription, sdesc);
    if (audio_desc == NULL) {
        fprintf(stderr, "ERROR: sample description is not audio\n");
        return;
    }

    AP4_Sample     sample;
    AP4_DataBuffer data;
    AP4_Ordinal    index = 0;
    while (AP4_SUCCEEDED(track->ReadSample(index, sample, data))) {
	    WriteSyncHeader(output, sample.GetSize());
        output->Write(data.GetData(), data.GetDataSize());
	    index++;
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
    if (!strcmp(*args, "--key")) {
        if (argc != 5) {
            fprintf(stderr, "ERROR: invalid command line\n");
            return 1;
        }
        ++args;
        if (AP4_ParseHex(*args++, key, 16)) {
            fprintf(stderr, "ERROR: invalid hex format for key\n");
            return 1;
        }
        key_option = true;
    }

    AP4_ByteStream* input  = NULL;
    AP4_File* input_file   = NULL;
    AP4_ByteStream* output = NULL;
    AP4_Movie*      movie  = NULL;
    AP4_Track*      audio_track = NULL;

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
    input_file = new AP4_File(*input);

    // get the movie
    AP4_SampleDescription* sample_description;
    movie = input_file->GetMovie();
    if (movie == NULL) {
        fprintf(stderr, "ERROR: no movie in file\n");
        goto end;
    }

    // get the audio track
    audio_track = movie->GetTrack(AP4_Track::TYPE_AUDIO);
    if (audio_track == NULL) {
        fprintf(stderr, "ERROR: no audio track found\n");
        goto end;
    }

    // check that the track is of the right type
    sample_description = audio_track->GetSampleDescription(0);
    if (sample_description == NULL) {
        fprintf(stderr, "ERROR: unable to parse sample description\n");
        goto end;
    }

    // show info
    AP4_Debug("Audio Track:\n");
    AP4_Debug("  duration: %u ms\n",  (int)audio_track->GetDurationMs());
    AP4_Debug("  sample count: %u\n", (int)audio_track->GetSampleCount());

    switch (sample_description->GetType()) {
        case AP4_SampleDescription::TYPE_AC4: {
            WriteSamples(audio_track, sample_description, output);
            return_value = 0;
            break;
        }

        case AP4_SampleDescription::TYPE_PROTECTED: 
            if (!key_option) {
                fprintf(stderr, "ERROR: encrypted tracks require a key\n");
                return_value = 1;
                break;
            }
            DecryptAndWriteSamples(audio_track, sample_description, key, output);
            result = 0;
            break;

        default:
            fprintf(stderr, "ERROR: unsupported sample type\n");
            return_value = 1;
            break;
    }

end:
    delete input_file;
    if (input) input->Release();
    if (output) output->Release();

    return return_value;
}
