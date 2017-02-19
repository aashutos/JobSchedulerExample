#include <jni.h>

#include <android/log.h>
#include <stdio.h>

#include <iostream>
#include <cstdlib>

// Defining templates
#define  LOG_TAG    "imgManipExample"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

bool comparePNGSignatures(char signature[8]);

PNGChunk parseChunk(FILE *pFILE);

PNGMetaData genPNGMetaData(char *data);

enum CriticalChunk {
    IHDR,PLTE,IDAT,IEND
};

enum ColourTypeEnum {
    GS=0,RGB=2,PLT=3,GS_ALPHA=4,RGB_ALPHA=6
};

struct PNGMetaData {
    char  name;
    int   width;
    int   height;
    char  bitDepth;
    char  colorType;
    char  compMethod;
    char  filtMethod;
    char  intMethod;
};

struct PNGChunk {
    char length[4];
    char type[4];
    char* data;
    char crc[4];
};

template <class T>
class ContiguousList: public StreamableList {
private:
    T *values;
    int index = 0;
    int length = 0;
public:
    int getLength() {
        return length;
    }

    int getIndex() {
        return index;
    }

    void incIndex() {
        index++;
    }

    T getNextEntry() {
        if (index >= length) {
            return NULL;
        } else {
            return values[index++];
        }
    }
};

template <class T>
class FragmentedSourceByteStream: public StreamableList {
private:
    ContiguousList<T> entries[];
    int index = 0;
    int length = 0;
    int step = sizeof(T);
public:
    T getNextEntry() {
        if (index*step >= getSize() || length == 0) {
            index = length;
            return NULL;
        }

        ContiguousList<T> entry = entries[index];
        if (entry.getIndex() >= entry.getLength()){
            index += 1;
            return getNextEntry();
        } else {
            entry.getNextEntry();
        }
        return entry;
    };

    void reset() {
        index = 0;
    }

    int getSize() {
        return length * step;
    }

    void pushItem(ContiguousList<T> entry) {

        if (!entry)
            return;

        if (length == 0) {
            entries = static_cast< ContiguousList<T>* > (malloc(sizeof(::ContiguousList<T>)));
            entries[0] = entry;
            length++;
        } else {
            entries = realloc(entries, ++length * sizeof(T));
            entries[length-1] = entry;
        }

        entries[length] = entry;
    }

    void clear() {
        free(entries);
        index = 0;
        length = 0;
    }
};

// jobject required to obtain jstring variable properly eventhough unused in method below
extern "C" JNIEXPORT void JNICALL Java_com_ntak_examples_jniexample_subscriber_BackgroundRenderResourceSubscriber_getBitmapJNI(JNIEnv* env, jobject obj, jstring filePath) {
    LOGI("JNI Method: 'getBitmapJNI' is being executed.");

    try {
      const char *loc = env->GetStringUTFChars(filePath,0);

      LOGI("JNI Method: Processing file: %s",loc);
      FILE* file = fopen(loc,"rw"); // TODO: jstring passing

      env->ReleaseStringUTFChars(filePath, loc);

      if (file == NULL) {
        LOGE("JNI Method: File not found.");
        fclose(file);
        return;
      }

      char READ_PNG_SIGNATURE[8];

      fread(READ_PNG_SIGNATURE, sizeof(char),8,file);

      bool isDefPNG = comparePNGSignatures(READ_PNG_SIGNATURE);

      if (!isDefPNG) {
        LOGE("JNI Method: File signature does not match that of a PNG File.");
        fclose(file);
        return;
      }

      LOGI("JNI Method: The file is a valid PNG File.");
      LOGI("Reading header chunk IHDR...");
      PNGChunk ihdrChunk = parseChunk(file);

      int isIHDR = strcmp("IHDR",ihdrChunk.type);

      if (isIHDR != 0) {
        LOGE("JNI Method: IHDR chunk not found in expected point within file. Please check file is not corrupt.");
        fclose(file);
        return;
      }

      PNGMetaData metaData = genPNGMetaData(*loc,ihdrChunk.data);

      // Read Palette into a Map/Dictionary lookup structure -> Investigate Boost Lib for data structures
      PNGChunk pltChunk = parseChunk(file);

      // Read chunks until palette chunk is reached or compressed data chunk
    while (pltChunk.type != "PLTE" || pltChunk.type != "IDAT") {
        pltChunk = parseChunk(file);

        if (pltChunk.type == "IEND") {
            LOGE("JNI Method: PLTE or IDAT chunk not found in expected point within file. Please check file is not corrupt.");
            fclose(file);
            return;
        }
    }

      // Load palette for Colour Type 3 => If not palette then throw error
    if (pltChunk.type != "PLTE" && metaData.colorType == PLT) {
          LOGE("JNI Method: PLTE chunk not found in expected point within file. Please check file is not corrupt.");
          fclose(file);
          return;
        }
    else {
      if (pltChunk.type == "PLTE") {
        // Parse palette here
      }
    }

    // Take chunk as next just in case an IDAT
    PNGChunk nextChunk = pltChunk;

    // If palette then get IDAT chunk
    while (pltChunk.type != "IDAT") {
        pltChunk = parseChunk(file);

        if (pltChunk.type == "IEND") {
            LOGE("JNI Method: IDAT chunk not found in expected point within file. Please check file is not corrupt.");
            fclose(file);
            return;
        }
    }

    // St IDAT point - mount consecutive IDATs into a stream
    while (nextChunk.type != NULL || nextChunk.type == "IDAT") {
      // Generate ByteStream source from chunks then decompress and manipulate

      nextChunk = parseChunk(file);
    }

    while (nextChunk.type != NULL || nextChunk.type != "IEND") {
      nextChunk = parseChunk(file);
    }

    if (nextChunk.type == NULL || nextChunk.type != "IEND") {
      LOGE("JNI Method: IEND chunk not found in expected point within file. Please check file is not corrupt.");
      fclose(file);
      return;
    }
      fclose(file);

      // Read in data from file -> Compressed byte stream -> Uncompressed (raw) byte stream -> Parse and substitute value (Using metadata
      // Don't care about interlace atm -> Just want to manipulate data.
      // Check # of pixels match dimensions specified?



      // Re-compress data post manipulation -> write stream to file

      // Append auxiliary chunk with details stating manipulation has occurred.

    } catch (std::exception e) {
      LOGE("JNI Method: Exception occurred. See stacktrace for details. Details: %s",e.what());
      return;
  }
    LOGI("JNI Method: 'getBitmapJNI' has executed successfully.");
}

PNGMetaData genPNGMetaData(char loc, char* data) { // Validate this -> Do we need to place checks for invalid files
  PNGMetaData metaData;
  metaData.name = loc;
  metaData.width = ((int[]) data)[0]; // 32 bit step as cast to integer
  metaData.height = ((int[]) data)[1];
  metaData.bitDepth = ((char[]) data)[8]; // 8 bit step as cast to char
  metaData.colorType = ((char[]) data)[9];
  metaData.compMethod = ((char[]) data)[10];
  metaData.filtMethod = ((char[]) data)[11];
  metaData.intMethod = ((char[]) data)[12];
  return metaData;
}

// 1. Utilise Google Test Framework -> Unit test
// 2. Break out code into sub modules - more granular structure
// 3. Break out header file from cpp file

PNGChunk parseChunk(FILE *pFILE) { // Validate this - Do we need to place checks for invalid files
  PNGChunk chunk;
  fread(chunk.length,sizeof(char),4,pFILE);
  if (feof(pFILE) != 0 || ferror(pFILE))
    return chunk;
  fread(chunk.type,sizeof(char),4,pFILE);
  fread(chunk.data,sizeof(char),atoi(chunk.length),pFILE);
  fread(chunk.crc,sizeof(char),4,pFILE);
  return chunk;
}

bool comparePNGSignatures(char signature[8]) {
  unsigned char PNG_SIGNATURE[8] = {0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A};

  int i = 0;
  for (i = 0; i < sizeof(PNG_SIGNATURE)/sizeof(*PNG_SIGNATURE); i++) {
    if (PNG_SIGNATURE[i] != signature[i])
      return false;
  }
  return true;
}