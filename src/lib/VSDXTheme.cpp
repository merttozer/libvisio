/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libvisio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "VSDXTheme.h"

#include <memory>

#include "VSDXMLTokenMap.h"
#include "libvisio_utils.h"
#include "libvisio_xml.h"

using std::shared_ptr;

namespace libvisio {

VSDXVariationClrScheme::VSDXVariationClrScheme() : m_varColor{} {}

VSDXClrScheme::VSDXClrScheme() : m_dk1(), m_lt1(), m_dk2(), m_lt2(), m_accent{}, m_hlink(), m_folHlink(), m_bkgnd(), m_variationClrSchemeLst() {}

VSDXFont::VSDXFont() : m_latinTypeFace(), m_eaTypeFace(), m_csTypeFace(), m_typeFaces() {}

VSDXFontScheme::VSDXFontScheme() : m_majorFont(), m_minorFont(), m_schemeId(0) {}

VSDXTheme::VSDXTheme() : m_clrScheme(), m_fontScheme(), m_fillStyleLst(6) {}

VSDXTheme::~VSDXTheme() = default;

int VSDXTheme::getElementToken(xmlTextReaderPtr reader) {
  return VSDXMLTokenMap::getTokenId(xmlTextReaderConstName(reader));
}

bool VSDXTheme::parse(librevenge::RVNGInputStream* input) {
  if (!input) return false;
  auto reader = xmlReaderForStream(input, nullptr, false);
  if (!reader) return false;

  try {
    int ret = xmlTextReaderRead(reader.get());
    while (ret == 1) {
      int tokenId = getElementToken(reader.get());
      switch (tokenId) {
        case XML_A_CLRSCHEME: readClrScheme(reader.get()); break;
        case XML_A_FONTSCHEME: readFontScheme(reader.get()); break;
        case XML_A_FMTSCHEME: readFmtScheme(reader.get()); break;
        default: break;
      }
      ret = xmlTextReaderRead(reader.get());
    }
  } catch (...) {
    return false;
  }
  return true;
}

boost::optional<Colour> VSDXTheme::readSrgbClr(xmlTextReaderPtr reader) {
  if (getElementToken(reader) == XML_A_SRGBCLR) {
    const shared_ptr<xmlChar> val(xmlTextReaderGetAttribute(reader, BAD_CAST("val")), xmlFree);
    if (val) {
      try {
        return xmlStringToColour(val);
      } catch (const XmlParserException&) {}
    }
  }
  return boost::optional<Colour>();
}

boost::optional<Colour> VSDXTheme::readSysClr(xmlTextReaderPtr reader) {
  if (getElementToken(reader) == XML_A_SYSCLR) {
    const shared_ptr<xmlChar> lastClr(xmlTextReaderGetAttribute(reader, BAD_CAST("lastClr")), xmlFree);
    if (lastClr) {
      try {
        return xmlStringToColour(lastClr);
      } catch (const XmlParserException&) {}
    }
  }
  return boost::optional<Colour>();
}

void VSDXTheme::readFontScheme(xmlTextReaderPtr reader) {
  int ret, tokenId, tokenType;
  do {
    ret = xmlTextReaderRead(reader);
    tokenId = getElementToken(reader);
    tokenType = xmlTextReaderNodeType(reader);
    switch (tokenId) {
      case XML_A_MAJORFONT: readFont(reader, tokenId, m_fontScheme.m_majorFont); break;
      case XML_A_MINORFONT: readFont(reader, tokenId, m_fontScheme.m_minorFont); break;
      default: break;
    }
  } while ((tokenId != XML_A_FONTSCHEME || tokenType != XML_READER_TYPE_END_ELEMENT) && ret == 1);
}

void VSDXTheme::readFont(xmlTextReaderPtr reader, int idToken, VSDXFont& font) {
  int ret, tokenId, tokenType;
  do {
    ret = xmlTextReaderRead(reader);
    tokenId = getElementToken(reader);
    tokenType = xmlTextReaderNodeType(reader);
    switch (tokenId) {
      case XML_A_LATIN: readTypeFace(reader, font.m_latinTypeFace); break;
      case XML_A_EA: readTypeFace(reader, font.m_eaTypeFace); break;
      case XML_A_CS: readTypeFace(reader, font.m_csTypeFace); break;
      case XML_A_FONT: {
        int script;
        librevenge::RVNGString typeFace;
        if (readTypeFace(reader, script, typeFace) && !typeFace.empty())
          font.m_typeFaces[script] = std::move(typeFace);
        break;
      }
      default: break;
    }
  } while ((idToken != tokenId || tokenType != XML_READER_TYPE_END_ELEMENT) && ret == 1);
}

bool VSDXTheme::readTypeFace(xmlTextReaderPtr reader, librevenge::RVNGString& typeFace) {
  const shared_ptr<xmlChar> sTypeFace(xmlTextReaderGetAttribute(reader, BAD_CAST("typeface")), xmlFree);
  if (sTypeFace) {
    typeFace = librevenge::RVNGString(reinterpret_cast<const char*>(sTypeFace.get()));
  }
  return bool(sTypeFace);
}

bool VSDXTheme::readTypeFace(xmlTextReaderPtr reader, int& script, librevenge::RVNGString& typeFace) {
  const shared_ptr<xmlChar> sScript(xmlTextReaderGetAttribute(reader, BAD_CAST("script")), xmlFree);
  bool knownScript = false;
  if (sScript) {
    int token = VSDXMLTokenMap::getTokenId(sScript.get());
    knownScript = token != XML_TOKEN_INVALID;
    if (knownScript) script = token;
  }
  return readTypeFace(reader, typeFace) && knownScript;
}

void VSDXTheme::readClrScheme(xmlTextReaderPtr reader) {
  m_clrScheme.m_variationClrSchemeLst.clear();
  int ret, tokenId, tokenType;
  do {
    ret = xmlTextReaderRead(reader);
    tokenId = getElementToken(reader);
    tokenType = xmlTextReaderNodeType(reader);
    switch (tokenId) {
      case XML_A_DK1: readThemeColour(reader, tokenId, m_clrScheme.m_dk1); break;
      case XML_A_DK2: readThemeColour(reader, tokenId, m_clrScheme.m_dk2); break;
      case XML_A_LT1: readThemeColour(reader, tokenId, m_clrScheme.m_lt1); break;
      case XML_A_LT2: readThemeColour(reader, tokenId, m_clrScheme.m_lt2); break;
      case XML_A_ACCENT1: readThemeColour(reader, tokenId, m_clrScheme.m_accent[0]); break;
      case XML_A_ACCENT2: readThemeColour(reader, tokenId, m_clrScheme.m_accent[1]); break;
      case XML_A_ACCENT3: readThemeColour(reader, tokenId, m_clrScheme.m_accent[2]); break;
      case XML_A_ACCENT4: readThemeColour(reader, tokenId, m_clrScheme.m_accent[3]); break;
      case XML_A_ACCENT5: readThemeColour(reader, tokenId, m_clrScheme.m_accent[4]); break;
      case XML_A_ACCENT6: readThemeColour(reader, tokenId, m_clrScheme.m_accent[5]); break;
      case XML_A_HLINK: readThemeColour(reader, tokenId, m_clrScheme.m_hlink); break;
      case XML_A_FOLHLINK: readThemeColour(reader, tokenId, m_clrScheme.m_folHlink); break;
      case XML_VT_BKGND: readThemeColour(reader, tokenId, m_clrScheme.m_bkgnd); break;
      case XML_VT_VARIATIONCLRSCHEMELST: readVariationClrSchemeLst(reader); break;
      default: break;
    }
  } while ((tokenId != XML_A_CLRSCHEME || tokenType != XML_READER_TYPE_END_ELEMENT) && ret == 1);
}

bool VSDXTheme::readThemeColour(xmlTextReaderPtr reader, int idToken, Colour& clr) {
  int ret, tokenId, tokenType;
  boost::optional<Colour> colour;
  do {
    ret = xmlTextReaderRead(reader);
    tokenId = getElementToken(reader);
    tokenType = xmlTextReaderNodeType(reader);
    switch (tokenId) {
      case XML_A_SRGBCLR: colour = readSrgbClr(reader); break;
      case XML_A_SYSCLR: colour = readSysClr(reader); break;
      default: break;
    }
  } while ((idToken != tokenId || tokenType != XML_READER_TYPE_END_ELEMENT) && ret == 1);

  if (colour) {
    clr = *colour;
    return true;
  }
  return false;
}

void VSDXTheme::readVariationClrSchemeLst(xmlTextReaderPtr reader) {
  int ret, tokenId, tokenType;
  do {
    ret = xmlTextReaderRead(reader);
    tokenId = getElementToken(reader);
    tokenType = xmlTextReaderNodeType(reader);
    if (tokenId == XML_VT_VARIATIONCLRSCHEME) {
      VSDXVariationClrScheme varClrSch;
      readVariationClrScheme(reader, varClrSch);
      m_clrScheme.m_variationClrSchemeLst.push_back(std::move(varClrSch));
    }
  } while ((tokenId != XML_VT_VARIATIONCLRSCHEMELST || tokenType != XML_READER_TYPE_END_ELEMENT) && ret == 1);
}

void VSDXTheme::readVariationClrScheme(xmlTextReaderPtr reader, VSDXVariationClrScheme& varClrSch) {
  int ret, tokenId, tokenType;
  do {
    ret = xmlTextReaderRead(reader);
    tokenId = getElementToken(reader);
    tokenType = xmlTextReaderNodeType(reader);
    switch (tokenId) {
      case XML_VT_VARCOLOR1: readThemeColour(reader, tokenId, varClrSch.m_varColor[0]); break;
      case XML_VT_VARCOLOR2: readThemeColour(reader, tokenId, varClrSch.m_varColor[1]); break;
      case XML_VT_VARCOLOR3: readThemeColour(reader, tokenId, varClrSch.m_varColor[2]); break;
      case XML_VT_VARCOLOR4: readThemeColour(reader, tokenId, varClrSch.m_varColor[3]); break;
      case XML_VT_VARCOLOR5: readThemeColour(reader, tokenId, varClrSch.m_varColor[4]); break;
      case XML_VT_VARCOLOR6: readThemeColour(reader, tokenId, varClrSch.m_varColor[5]); break;
      case XML_VT_VARCOLOR7: readThemeColour(reader, tokenId, varClrSch.m_varColor[6]); break;
      default: break;
    }
  } while ((tokenId != XML_VT_VARIATIONCLRSCHEME || tokenType != XML_READER_TYPE_END_ELEMENT) && ret == 1);
}

boost::optional<Colour> VSDXTheme::getThemeColour(unsigned value, unsigned variationIndex) const {
  if (value < 100) {
    switch (value) {
      case 0: return m_clrScheme.m_dk1;
      case 1: return m_clrScheme.m_lt1;
      case 2: return m_clrScheme.m_accent[0];
      case 3: return m_clrScheme.m_accent[1];
      case 4: return m_clrScheme.m_accent[2];
      case 5: return m_clrScheme.m_accent[3];
      case 6: return m_clrScheme.m_accent[4];
      case 7: return m_clrScheme.m_accent[5];
      case 8: return m_clrScheme.m_bkgnd;
      default: break;
    }
  } else if (!m_clrScheme.m_variationClrSchemeLst.empty()) {
    if (variationIndex >= m_clrScheme.m_variationClrSchemeLst.size())
      variationIndex = 0;
    switch (value) {
      case 100: case 200: return m_clrScheme.m_variationClrSchemeLst[variationIndex].m_varColor[0];
      case 101: case 201: return m_clrScheme.m_variationClrSchemeLst[variationIndex].m_varColor[1];
      case 102: case 202: return m_clrScheme.m_variationClrSchemeLst[variationIndex].m_varColor[2];
      case 103: case 203: return m_clrScheme.m_variationClrSchemeLst[variationIndex].m_varColor[3];
      case 104: case 204: return m_clrScheme.m_variationClrSchemeLst[variationIndex].m_varColor[4];
      case 105: case 205: return m_clrScheme.m_variationClrSchemeLst[variationIndex].m_varColor[5];
      case 106: case 206: return m_clrScheme.m_variationClrSchemeLst[variationIndex].m_varColor[6];
      default: break;
    }
  }
  return boost::optional<Colour>();
}

void VSDXTheme::readFmtScheme(xmlTextReaderPtr reader) {
  int ret, tokenId, tokenType;
  do {
    ret = xmlTextReaderRead(reader);
    tokenId = getElementToken(reader);
    tokenType = xmlTextReaderNodeType(reader);
    if (tokenId == XML_A_FILLSTYLELST) {
      readFillStyleLst(reader);
    }
  } while ((tokenId != XML_A_FMTSCHEME || tokenType != XML_READER_TYPE_END_ELEMENT) && ret == 1);
}

void VSDXTheme::skipUnimplemented(xmlTextReaderPtr reader, int idToken) {
  int ret, tokenId, tokenType;
  do {
    ret = xmlTextReaderRead(reader);
    tokenId = getElementToken(reader);
    tokenType = xmlTextReaderNodeType(reader);
  } while ((idToken != tokenId || tokenType != XML_READER_TYPE_END_ELEMENT) && ret == 1);
}

void VSDXTheme::readFillStyleLst(xmlTextReaderPtr reader) {
  int ret = xmlTextReaderRead(reader);
  int tokenId = getElementToken(reader);
  int tokenType = xmlTextReaderNodeType(reader);
  int i = 0;
  while ((tokenId != XML_A_FILLSTYLELST || tokenType != XML_READER_TYPE_END_ELEMENT) && ret == 1) {
    switch (tokenId) {
      case XML_A_SOLIDFILL: {
        Colour colour;
        if (readThemeColour(reader, tokenId, colour)) {
          m_fillStyleLst[i] = colour;
        }
        break;
      }
      default: skipUnimplemented(reader, tokenId); break;
    }
    ret = xmlTextReaderRead(reader);
    tokenId = getElementToken(reader);
    tokenType = xmlTextReaderNodeType(reader);
  }
}

boost::optional<Colour> VSDXTheme::getFillStyleColour(unsigned value) const {
  if (value == 0 || value > m_fillStyleLst.size())
    return boost::optional<Colour>();
  return m_fillStyleLst[value - 1];
}

} // namespace libvisio