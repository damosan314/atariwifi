#include "printer.h"

//pdf routines
void sioPrinter::pdf_header()
{
  pdf_Y = 0;
  pdf_X = 0;
  pdf_pageCounter = 0;
  _file->printf("%%PDF-1.4\n");
  // first object: catalog of pages
  pdf_objCtr = 1;
  objLocations[pdf_objCtr] = _file->position();
  _file->printf("1 0 obj\n<</Type /Catalog /Pages 2 0 R>>\nendobj\n");
  // object 2 0 R is printed at bottom of PDF before xref
  this->pdf_fonts(); // pdf_fonts is virtual function, call by pointer
}

void sioPrinter::pdf_fonts()
{
  // 3rd object: font catalog
  pdf_objCtr = 3;
  objLocations[pdf_objCtr] = _file->position();
  _file->printf("3 0 obj\n<</Font << /F1 4 0 R >>>>\nendobj\n");

  // 1027 standard font
  pdf_objCtr = 4;
  objLocations[pdf_objCtr] = _file->position();
  _file->printf("4 0 obj\n<</Type /Font /Subtype /Type1 /BaseFont /Courier /Encoding /WinAnsiEncoding>>\nendobj\n");
}

void atari1027::pdf_fonts()
/*
5 0 obj
<</Type/Font/Subtype/TrueType/Name/F1/BaseFont/PrestigeEliteNormal/Encoding/WinAnsiEncoding/FontDescriptor 6 0 R/FirstChar 32/LastChar 252/Widths 7 0 R>>
endobj
6 0 obj
<</Type/FontDescriptor/FontName/PrestigeEliteNormal/Flags 33/ItalicAngle 0/Ascent 662/Descent -216/CapHeight 662/AvgWidth 600/MaxWidth 600/FontWeight 400/XHeight 250/StemV 60/FontBBox[ -2 -216 625 662] >>
endobj
7 0 obj
[ 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600
  600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600
  600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600
  600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600
  600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600
  600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600
  600 600 600 600 600 600 600 600]
endobj
*/
{
  // 3rd object: font catalog
  pdf_objCtr = 3;
  objLocations[pdf_objCtr] = _file->position();
  _file->printf("3 0 obj\n<</Font <</F1 4 0 R /F2 7 0 R>>>>\nendobj\n");

  // 1027 standard font
  pdf_objCtr = 4;
  objLocations[pdf_objCtr] = _file->position();
  _file->printf("4 0 obj\n<</Type/Font/Subtype/TrueType/Name/F1/BaseFont/PrestigeEliteNormal/Encoding/WinAnsiEncoding/FontDescriptor 5 0 R/FirstChar 32/LastChar 252/Widths 6 0 R>>\nendobj\n");
  pdf_objCtr = 5;
  objLocations[pdf_objCtr] = _file->position();
  _file->printf("5 0 obj\n<</Type/FontDescriptor/FontName/PrestigeEliteNormal/Flags 33/ItalicAngle 0/Ascent 662/Descent -216/CapHeight 662/AvgWidth 600/MaxWidth 600/FontWeight 400/XHeight 250/StemV 60/FontBBox[ -2 -216 625 662]>>\nendobj\n");
  pdf_objCtr = 6;
  objLocations[pdf_objCtr] = _file->position();
  _file->printf("6 0 obj\n[");
  for (int i = 32; i < 253; i++)
  {
    _file->printf(" 600");
    if ((i - 31) % 32 == 0)
      _file->printf("\n");
  }
  _file->printf(" ]\nendobj\n");

  // symbol font to put in arrows
  pdf_objCtr = 7;
  objLocations[pdf_objCtr] = _file->position();
  _file->printf("7 0 obj\n<</Type /Font /Subtype /Type1 /BaseFont /Symbol /Encoding /WinAnsiEncoding>>\nendobj\n");
}

void atari820::pdf_fonts()
/*
5 0 obj
<</Type/Font/Subtype/TrueType/Name/F1/BaseFont/mono5by7_820/Encoding/WinAnsiEncoding/FontDescriptor 6 0 R/FirstChar 32/LastChar 127/Widths 7 0 R>>
endobj
6 0 obj
<</Type/FontDescriptor/FontName/mono5by7_820/Flags 33/ItalicAngle 0/Ascent 700/Descent 0/CapHeight 700/AvgWidth 600/MaxWidth 600/FontWeight 400/XHeight 250/StemV 50/FontBBox[ 0  0 700 700] >>
endobj
7 0 obj
[600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600
 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600
 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600 600  ] 
endobj
*/
{
  // 3rd object: font catalog
  pdf_objCtr = 3;
  objLocations[pdf_objCtr] = _file->position();
  _file->printf("3 0 obj\n<</Font << /F1 4 0 R >>>>\nendobj\n");

  // 1027 standard font
  pdf_objCtr = 4;
  objLocations[pdf_objCtr] = _file->position();
  _file->printf("4 0 obj\n<</Type/Font/Subtype/TrueType/Name/F1/BaseFont/mono5by7ascii/Encoding/WinAnsiEncoding/FontDescriptor 5 0 R/FirstChar 32/LastChar 127/Widths 6 0 R>>\nendobj\n");
  pdf_objCtr = 5;
  objLocations[pdf_objCtr] = _file->position();
  _file->printf("5 0 obj\n<</Type/FontDescriptor/FontName/mono5by7ascii/Flags 33/ItalicAngle 0/Ascent 700/Descent 0/CapHeight 700/AvgWidth 600/MaxWidth 600/FontWeight 400/XHeight 250/StemV 50/FontBBox[0 0 700 700] >>\nendobj\n");
  pdf_objCtr = 6;
  objLocations[pdf_objCtr] = _file->position();
  _file->printf("6 0 obj\n[");
  for (int i = 32; i < 128; i++)
  {
    _file->printf(" 600");
    if ((i - 31) % 32 == 0)
      _file->printf("\n");
  }
  _file->printf(" ]\nendobj\n");
}

void sioPrinter::pdf_xref()
{
  int max_objCtr = pdf_objCtr;
  pdf_objCtr = 2;
  objLocations[pdf_objCtr] = _file->position(); // hard code page catalog as object #2
  _file->printf("2 0 obj\n<</Type /Pages /Kids [ ");
  for (int i = 0; i < pdf_pageCounter; i++)
  {
    _file->printf("%d 0 R ", pageObjects[i]);
  }
  _file->printf("] /Count %d>>\nendobj\n", pdf_pageCounter);
  size_t xref = _file->position();
  max_objCtr++;
  _file->printf("xref\n");
  _file->printf("0 %u\n", max_objCtr);
  _file->printf("0000000000 65535 f\n");
  for (int i = 1; i < max_objCtr; i++)
  {
    _file->printf("%010u 00000 n\n", objLocations[i]);
  }
  _file->printf("trailer <</Size %u/Root 1 0 R>>\n", max_objCtr);
  _file->printf("startxref\n");
  _file->printf("%u\n", xref);
  _file->printf("%%%%EOF\n");
}

void sioPrinter::pdf_new_page()
{ // open a new page
  pdf_objCtr++;
  pageObjects[pdf_pageCounter] = pdf_objCtr;
  objLocations[pdf_objCtr] = _file->position();
  _file->printf("%d 0 obj\n<</Type /Page /Parent 2 0 R /Resources 3 0 R /MediaBox [0 0 %g %g] /Contents [ ", pdf_objCtr, pageWidth, pageHeight);
  pdf_objCtr++; // increment for the contents stream object
  _file->printf("%d 0 R ", pdf_objCtr);
  _file->printf("]>>\nendobj\n");

  // open new content stream and text object
  objLocations[pdf_objCtr] = _file->position();
  _file->printf("%d 0 obj\n<</Length ", pdf_objCtr);
  idx_stream_length = _file->position();
  _file->printf("00000>>\nstream\n");
  idx_stream_start = _file->position();
  _file->printf("BT\n");

  TOPflag = false;
  // set default font for the page
  if (fontHorizontalScaling != 100)
    _file->printf("/F%u %u Tf %g Tz\n", fontNumber, fontSize, fontHorizontalScaling);
  else
    _file->printf("/F%u %u Tf\n", fontNumber, fontSize);
  _file->printf("%g %g Td\n", leftMargin, pageHeight);
  pdf_Y = pageHeight; // reset print roller to top of page
  pdf_X = 0;          // set carriage to LHS
  BOLflag = true;
}

void sioPrinter::pdf_end_page()
{
  // close text object & stream
  if (!BOLflag)
    pdf_end_line();
  _file->printf("ET\n");
  idx_stream_stop = _file->position();
  _file->printf("endstream\nendobj\n");
  size_t idx_temp = _file->position();
  _file->seek(idx_stream_length);
  _file->printf("%5u", (idx_stream_stop - idx_stream_start));
  _file->seek(idx_temp);
  // set counters
  pdf_pageCounter++;
  TOPflag = true;
}

void sioPrinter::pdf_new_line()
{
  // position new line and start text string array
  _file->printf("0 %g Td [(", -lineHeight);
  pdf_X = 0; // reinforce?
  BOLflag = false;
}

void sioPrinter::pdf_end_line()
{
  _file->printf(")]TJ\n"); // close the line
  pdf_Y -= lineHeight;     // line feed
  pdf_X = 0;               // CR
  BOLflag = true;
}

void sioPrinter::pdf_handle_char(byte c)
{
  // simple ASCII printer
  if (c > 31 && c < 127)
  {
    if (c == BACKSLASH || c == LEFTPAREN || c == RIGHTPAREN)
      _file->write(BACKSLASH);
    _file->write(c);

    pdf_X += charWidth; // update x position
  }
}

void atari1027::pdf_handle_char(byte c)
{
  if (escMode)
  {
    // Atari 1027 escape codes:
    // CTRL-O - start underscoring        15
    // CTRL-N - stop underscoring         14  - note in T1027.BAS there is a case of 27 14
    // ESC CTRL-Y - start underscoring    27  25
    // ESC CTRL-Z - stop underscoring     27  26
    // ESC CTRL-W - start international   27  23
    // ESC CTRL-X - stop international    27  24
    if (c == 25)
      uscoreFlag = true;
    if (c == 26)
      uscoreFlag = false;
    if (c == 23)
      intlFlag = true;
    if (c == 24)
      intlFlag = false;
    escMode = false;
  }
  else if (c == 15)
    uscoreFlag = true;
  else if (c == 14)
    uscoreFlag = false;
  else if (c == 27)
    escMode = true;
  else
  { // maybe printable character
    //printable characters for 1027 Standard Set + a few more >123 -- see mapping atari on ATASCII
    if (intlFlag && (c < 32 || c == 123))
    {
      // not sure about ATASCII 96.
      // todo: Codes 28-31 are arrows and require the symbol font
      if (c < 27)
        _file->write(intlchar[c]);
      else if (c == 123)
        _file->write(byte(196));
      else if (c > 27 && c < 32)
      {
        //_file->printf(")]TJ\n/F2 12 Tf (");
        _file->write(intlchar[c]); // Symbol font is not monospace
        //_file->printf(")Tj\n/F1 12 Tf\n[(");
      }

      pdf_X += charWidth; // update x position
    }
    else if (c > 31 && c < 127)
    {
      if (c == BACKSLASH || c == LEFTPAREN || c == RIGHTPAREN)
        _file->write(BACKSLASH);
      _file->write(c);

      if (uscoreFlag)
        _file->printf(")600(_"); // close text string, backspace, start new text string, write _

      pdf_X += charWidth; // update x position
    }
  }
}

void sioPrinter::pdf_add(std::string S)
{
  if (TOPflag)
    pdf_new_page();

  // loop through string
  for (int i = 0; i < S.length(); i++)
  {
    byte c = byte(S[i]);

    if (BOLflag && c == EOL)
      pdf_new_line();

    // check for EOL or if at end of line and need automatic CR
    if (!BOLflag && ((c == EOL) || (pdf_X > (printWidth - charWidth))))
      pdf_end_line();

    // start a new line if we need to
    if (BOLflag)
      pdf_new_line();

    // disposition the current byte
    this->pdf_handle_char(c);

#ifdef DEBUG_S
    printf("c: %3d  x: %6.2f  y: %6.2f  ", c, pdf_X, pdf_Y);
    //printf("TOP: %s  ", TOPflag ? "true " : "false");
    //printf("BOL: %s  ", BOLflag ? "true " : "false");
    printf("\n");
#endif
  }

  // if wrote last line, then close the page
  if (pdf_Y < lineHeight + bottomMargin)
    pdf_end_page();
}

void sioPrinter::svg_add(std::string S)
{
}

void sioPrinter::initPrinter(File *f, paper_t ty)
{
  _file = f;
  paperType = ty;
  if (paperType == PDF)
    pdf_header();
}

void sioPrinter::initPrinter(File *f)
{
  this->initPrinter(f, PDF);
}

void atari1027::initPrinter(File *f, paper_t ty)
{
  _file = f;
  paperType = ty;
  if (paperType == PDF)
  {
    uscoreFlag = false;
    intlFlag = false;
    escMode = false;
    pdf_header();
  }
}

void atari820::initPrinter(File *f, paper_t ty)
{

  _file = f;
  paperType = ty;
  pageWidth = 279.0;  // paper roll is 3 7/8" from page 6 of owners manual
  pageHeight = 792.0; // just use 11" for letter paper
  leftMargin = 19.5;  // fit print width on page width
  bottomMargin = 0.0;
  // dimensions from Table 1-1 of Atari 820 Field Service Manual
  printWidth = 240.0; // 3 1/3" wide printable area
  lineHeight = 12.0;  // 6 lines per inch
  charWidth = 6.0;    // 12 char per inch
  fontSize = 12;      // 6 lines per inch
  fontHorizontalScaling = 83.333;

  if (paperType == PDF)
  {
    sideFlag = false;
    pdf_header();
  }
}

void sioPrinter::pageEject()
{
  if (paperType == PDF)
  {
    if (TOPflag && pdf_pageCounter == 0)
      pdf_new_page();
    if (!BOLflag)
      pdf_end_line();
    // to do: close the text string array if !BOLflag
    if (!TOPflag || pdf_pageCounter == 0)
      pdf_end_page();
    pdf_xref();
  }
}

paper_t sioPrinter::getPaperType()
{
  return paperType;
}

void sioPrinter::writeBuffer(byte *B, int n)
{
  int i = 0;
  std::string output = std::string();

  switch (paperType)
  {
  case RAW:
    for (i = 0; i < n; i++)
    {
      _file->write(B[i]);
    }
    break;
  case TRIM:
    while (i < n)
    {
      _file->write(B[i]);
      if (B[i] == EOL)
        break;
      i++;
    }
    break;
  case ASCII:
    while (i < n)
    {
      if (B[i] == EOL)
      {
        _file->printf("\n");
        break;
      }
      if (B[i] > 31 && B[i] < 127)
        _file->write(B[i]);
      i++;
    }
    break;
  case PDF:
    while (i < n)
    {
      output.push_back(B[i]);
      if (B[i] == EOL)
        break;
      i++;
    }
    pdf_add(output);
    break;
  case SVG:
    while (i < n)
    {
      output.push_back(B[i]);
      if (B[i] == EOL)
        break;
      i++;
    }
    svg_add(output);
  }
}

// write for W commands
void sioPrinter::sio_write()
{
  byte n = 40;
  byte ck;

  memset(buffer, 0, n); // clear buffer

  // to do: size buffer based on AUX1:
  // Auxiliary Bytes 1
  // Normal Print (40 Char/Line) = 0x4E
  // Sideways Print (16 Char/Line) = 0x53
  // Other values in the 400/800 OS Manual
  // Normal 0x4E 'N'    40 chars
  // Sideways 0x53 'S'  29 chars
  // Wide 0x57 'W'      "not supported"
  // And the XL/XE OS ROM

  if (cmdFrame.aux1 == 'N')
    n = 40;
  else if (cmdFrame.aux1 == 'S')
    n = 29;

  // new

  ck = sio_to_peripheral(buffer, n);

  if (ck == sio_checksum(buffer, n))
  {
    writeBuffer(buffer, n);
    sio_complete();
  }
  else
  {
    sio_error();
  }

  // old
  //   SIO_UART.readBytes(buffer, n);
  // #ifdef DEBUG_S
  //   for (int z = 0; z < n; z++)
  //   {
  //     BUG_UART.print(buffer[z], DEC);
  //     BUG_UART.print(" ");
  //   }
  //   BUG_UART.println();
  // #endif
  //   ck = SIO_UART.read(); // Read checksum
  //   //delayMicroseconds(350);
  //   SIO_UART.write('A'); // Write ACK

  //   if (ck == sio_checksum(buffer, n))
  //   {
  //     writeBuffer(buffer, n);
  //     delayMicroseconds(DELAY_T5);
  //     SIO_UART.write('C');
  //     yield();
  //   }
}

// Status
void sioPrinter::sio_status()
{
  byte status[4];
  // byte ck;

  // status frame per Atari 820 service manual
  /* The printer controller will return a data frame to the computer
reflecting the status. The STATUS DATA frame is shown below:
DONE/ERROR FLAG
AUX. BYTE 1 from last WRITE COMMAND
DATA WRITE TIMEOUT
CHECKSUM
The FLAG byte contains information relating to the most recent
command prior to the status request and some controller constants.
The DATA WRITE Timeout equals the maximum time to print a
line of data assuming worst case controller produced Timeout
delay. This Timeout is associated with printer timeout
discussed earlier. 
And  from 400/800 OS ROM Manual
Command Status
Aux 1 Byte (typo says AUX2 byte)
Timeout
Unused

OS ROM Manual continues on Command Status:
bit 0 - invalid command frame
bit 1 - invalid data frame
bit 7 - intelligent controller (normally 0)
*/
  status[0] = 0;
  status[1] = lastAux1;
  status[2] = 5;
  status[3] = 0;

  sio_to_computer(status, sizeof(status), false);

  // ck = sio_checksum((byte *)&status, 4);

  // delayMicroseconds(DELAY_T5); // t5 delay
  // SIO_UART.write('C');         // Command always completes.
  // SIO_UART.flush();
  // delayMicroseconds(200);
  // //delay(1);

  // // Write data frame
  // for (int i = 0; i < 4; i++)
  //   SIO_UART.write(status[i]);

  // // Write checksum
  // SIO_UART.write(ck);
  // SIO_UART.flush();
  // delayMicroseconds(200);
}

// Process command
void sioPrinter::sio_process()
{
  switch (cmdFrame.comnd)
  {
  case 'W':
    sio_ack();
    sio_write();
    lastAux1 = cmdFrame.aux1;
    break;
  case 'S':
    sio_ack();
    sio_status();
    break;
  default:
    sio_nak();
  }
  // cmdState = WAIT;
  //cmdTimer = 0;
}
