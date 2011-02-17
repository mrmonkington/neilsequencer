/*
	This file is part of the Buzé base Buzz-library. 
	
	Please refer to LICENSE.TXT for details regarding usage.
*/

#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace file {


class StreamWriter {
public:
	StreamWriter() { }
	virtual ~StreamWriter() { }

	void write(unsigned int i) {
		writeBytes(&i, sizeof(unsigned int));
	}

	void write(unsigned long i) {
		writeBytes(&i, sizeof(unsigned long));
	}

	void write(int i) {
		writeBytes(&i, sizeof(int));
	}

	void write(unsigned short i) {
		writeBytes(&i, sizeof(unsigned short));
	}

	void write(short i) {
		writeBytes(&i, sizeof(short));
	}

	void write(unsigned char i) {
		writeBytes(&i, sizeof(unsigned char));
	}

	void write(char i) {
		writeBytes(&i, sizeof(char));
	}

	void write(float i) {
		writeBytes(&i, sizeof(float));
	}

	void write(double i) {
		writeBytes(&i, sizeof(double));
	}

	virtual void writeBytes(const void* v, unsigned int size)=0;

};

class StreamReader {
public:
	StreamReader() { }
	virtual ~StreamReader() { }

	virtual int read(unsigned int* v) {
		return this->readBytes(v, sizeof(unsigned int));
	}

	virtual int read(int* v) {
		return this->readBytes(v, sizeof(int));
	}

	virtual int read(unsigned short* v) {
		return this->readBytes(v, sizeof(unsigned short));
	}

	virtual int read(unsigned char* v) {
		return this->readBytes(v, sizeof(unsigned char));
	}

	virtual int read(char* v) {
		return this->readBytes(v, sizeof(char));
	}

	virtual int read(unsigned long* v) {
		return this->readBytes(v, sizeof(unsigned long));
	}

	virtual int read(float* v) {
		return this->readBytes(v, sizeof(float));
	}

	std::string readAsciiZ() {
		using namespace std;
		string sb;
		while (!eof()) {
			char c;
			read(&c);
			if (c==0)
				break;
			sb+=c;
		}
		return sb;
	}

	std::string readLine() {
		using namespace std;
		string sb;
		char c=0;
		while (!eof()) {
			read(&c);
			if (c==0) break;
			if (c=='\r' || c=='\n') break;
			sb+=c;
		}
		if (c=='\r' && peek()=='\n') {
			read(&c);	// skip 2nd byte in crlf
		}
		return sb;
	}

	virtual bool eof()=0;
	virtual char peek()=0;

	virtual int readBytes(void* v, unsigned int size)=0;
};


class FileBase {
protected:
public:
	FILE* f;
/*#if defined(_MSC_VER)
	HANDLE getHandle() {
		// pga buffering kan dette fucke - vi må kanskje seeke her?
		return (HANDLE)_get_osfhandle(f->_file);
	}
#endif*/
	FileBase() {
		f=0;
	}
	virtual ~FileBase() { }

	void seek(long pos, int mode=SEEK_SET) {
		if (!f) return ;
		fseek(f, pos, mode);
	}

	long position() {
		if (!f) return -1;
		return ftell(f);
	}

	void close() {
		if (!f) return ;
		fclose(f);
	}

};

class FileWriter 
	: public FileBase
	, public StreamWriter {
public:
	FileWriter() { }
	~FileWriter() { }
	bool create(const char* fn) {
		f=fopen(fn, "w+b");//_O_BINARY|_O_RDWR|_O_CREAT|_O_TRUNC, _S_IREAD | _S_IWRITE );
		if (!f) return false;
		return true;
	}

	bool open(const char* fn) {
		f=fopen(fn, "rb");//_O_BINARY|_O_RDWR, _S_IREAD | _S_IWRITE );
		if (!f) return false;
		return true;
	}

	void writeBytes(const void* p, size_t bytes) {
		fwrite(p, (unsigned int)bytes, 1, f);
	}


	void writeAsciiZ(const char* pc) {
		fwrite(pc, (unsigned int)strlen(pc), 1, f);
		char n=0;
		writeBytes(&n, 1);
	}

	void writeLine(std::string s) {
		fwrite(s.c_str(), (unsigned int)s.length(), 1, f);
		char n='\r';
		fwrite(&n, 1, 1, f);
		n='\n';
		fwrite(&n, 1, 1, f);
	}
};

class StringWriter : public StreamWriter {
	//std::string text;
	char* text;
	size_t bufferSize;
	size_t ofs;

public:
	StringWriter(bool isZeroTerminated=true) {
		text=0;
		bufferSize=0;
		ofs=0;
	}

	~StringWriter() {
		if (text)
			delete[] text;
	}

	int position() {
		return (int)ofs;
	}


	void grow() {
		int newSize=(int)((float)bufferSize*1.25f);
		if (newSize==0) newSize=128;
		char* newBuffer=new char[newSize];
		if (text) {
			memcpy(newBuffer, text, bufferSize);
			delete[] text;
		}
		bufferSize=newSize;
		text=newBuffer;
	}


	void writeBytes(const void* p, size_t bytes) {
		while (!text || ofs+bytes>=bufferSize) grow();	// not too cool with large buffers

		memcpy(text+ofs, p, bytes);
		ofs+=bytes;
	}

	const char* getString() { return text; }
};

class StringReader : public StreamReader {
	size_t ofs;
	const char* text;
public:
	bool isZeroTerminated;

	StringReader(const char* text, bool isZeroTerminated=true) {
		this->text=text;
		ofs=0;
		this->isZeroTerminated=isZeroTerminated;
	}
	int readBytes(void* v, size_t size) {
		memcpy(v, text+ofs, size);
		ofs+=size;
		return (int)size;
	}
	bool eof() {
		if (isZeroTerminated && ofs>=strlen(text)) return true;
		return false;	// neverending buffer
	}
	char peek() {
		if (eof()) return -1;
		return text[ofs];
	}
};

class FileReader
	: public FileBase
	, public StreamReader
{
public:
	FileReader() { }
	~FileReader() { }
	bool open(const char* fn) {
		f=fopen(fn, "rb");//_O_BINARY|_O_RDONLY, _S_IREAD | _S_IWRITE );
		if (!f) return false;
		return true;
	}
/*
#if defined(_MSC_VER)
	HANDLE getHandle() {
		return (HANDLE)_get_osfhandle(f->_file);
	}
#endif*/

	int readBytes(void* v, unsigned int size) {
		int pos=position();
		fread(v, size, 1, f);
		return position()-pos;
	}

	char peek() {
		char c;
		if (!read(&c)) return 0;
		seek(-1, SEEK_CUR);
		return c;
	}

	bool eof() {
		return feof(f)!=0;
	}


};

}