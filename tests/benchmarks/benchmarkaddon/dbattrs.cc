//------------------------------------------------------------------------------
//  dbattrs.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "benchmarkaddon/dbattrs.h"

namespace Attr
{
    // this is taken from the DSA NPC table
    DefineGuid(Guid, 'GUID', ReadOnly);
    DefineString(Level, 'LEVL', ReadWrite);
    DefineString(Id, 'ID__', ReadOnly);
    DefineString(Graphics, 'GFX_', ReadOnly);
    DefineBool(Rot180, 'R180', ReadOnly);
    DefineString(CharacterSet, 'CHST', ReadOnly);
    DefineString(AnimSet, 'ANST', ReadOnly);
    DefineString(SoundSet, 'SDST', ReadOnly);
    DefineString(Placeholder, 'PLCH', ReadOnly);
    DefineString(Name, 'NAME', ReadWrite);
    DefineString(EntityGroup, 'EGRP', ReadWrite);
    DefineString(Faction, 'FCTN', ReadOnly);
    DefineString(Behaviour, 'BHVR', ReadOnly);
    DefineFloat(MaxVelocity, 'MVEL', ReadOnly);
    DefineString(LookatText, 'LOKT', ReadWrite);
    DefineBool(NewHero, 'NWHR', ReadWrite);
    DefineFloat(AggroRadius, 'AGRD', ReadOnly);
    DefineString(SetupStorage, 'STST', ReadWrite);
    DefineString(SetupEquipment, 'STEQ', ReadWrite);
    DefineString(InventoryType, 'INVT', ReadWrite);
    
    DefineInt(MU, 'MU__', ReadWrite);
    DefineInt(KL, 'KL__', ReadWrite);
    DefineInt(IN, 'IN__', ReadWrite);
    DefineInt(CH, 'CH__', ReadWrite);
    DefineInt(FF, 'FF__', ReadWrite);
    DefineInt(GE, 'GE__', ReadWrite);
    DefineInt(KO, 'KO__', ReadWrite);
    DefineInt(KK, 'KK__', ReadWrite);
    
    DefineInt(TaArmbrust, 'ARMB', ReadWrite);
    DefineInt(TaATArmbrust, 'AARM', ReadWrite);
    DefineInt(TaPAArmbrust, 'PARM', ReadWrite);
    DefineInt(TaDolche, 'DOLC', ReadWrite);
    DefineInt(TaATDolche, 'ADOL', ReadWrite);
    DefineInt(TaPADolche, 'PDOL', ReadWrite);
    DefineInt(TaFechtwaffen, 'FECH', ReadWrite);
    DefineInt(TaATFechtwaffen, 'AFEC', ReadWrite);
    DefineInt(TaPAFechtwaffen, 'PFEC', ReadWrite);
    DefineInt(TaHiebwaffen, 'HIEB', ReadWrite);
    DefineInt(TaATHiebwaffen, 'AHIE', ReadWrite);
    DefineInt(TaPAHiebwaffen, 'PHIE', ReadWrite);
    DefineInt(TaInfanteriewaffen, 'INFW', ReadWrite);
    DefineInt(TaATInfanteriewaffen, 'AINF', ReadWrite);
    DefineInt(TaPAInfanteriewaffen, 'PINF', ReadWrite);
    DefineInt(TaKettenwaffen, 'KETT', ReadWrite);
    DefineInt(TaATKettenwaffen, 'AKET', ReadWrite);
    DefineInt(TaPAKettenwaffen, 'PKET', ReadWrite);
    DefineInt(TaSaebel, 'SAEB', ReadWrite);
    DefineInt(TaATSaebel, 'ASAE', ReadWrite);
    DefineInt(TaPASaebel, 'PSAE', ReadWrite);
    DefineInt(TaSchwerter, 'SCHW', ReadWrite);
    DefineInt(TaATSchwerter, 'ASCH', ReadWrite);
    DefineInt(TaPASchwerter, 'PSCH', ReadWrite);
    DefineInt(TaSpeere, 'SPEE', ReadWrite);
    DefineInt(TaATSpeere, 'ASPE', ReadWrite);
    DefineInt(TaPASpeere, 'PSPE', ReadWrite);
    DefineInt(TaStaebe, 'STAE', ReadWrite);
    DefineInt(TaATStaebe, 'ASTA', ReadWrite);
    DefineInt(TaPAStaebe, 'PSTA', ReadWrite);
    DefineInt(TaZwHiebwaffen, 'ZWHB', ReadWrite);
    DefineInt(TaATZwHiebwaffen, 'AZWH', ReadWrite);
    DefineInt(TaPAZwHiebwaffen, 'PZWH', ReadWrite);
    DefineInt(TaZwSchwerter, 'ZWHS', ReadWrite);
    DefineInt(TaATZwSchwerter, 'AZWS', ReadWrite);
    DefineInt(TaPAZwSchwerter, 'PZWS', ReadWrite);
    DefineInt(TaRaufen, 'RAUF', ReadWrite);
    DefineInt(TaATRaufen, 'ARAU', ReadWrite);
    DefineInt(TaPARaufen, 'PRAU', ReadWrite);
    DefineInt(TaBogen, 'BOGN', ReadWrite);
    DefineInt(TaWurfbeile, 'WRFB', ReadWrite);
    DefineInt(TaWurfmesser, 'WRFM', ReadWrite);
    DefineInt(TaWurfspeer, 'WRFS', ReadWrite);

    DefineInt(TaGaukeleien, 'GAUK', ReadWrite);
    DefineInt(TaKoerperbeherrschung, 'KBHR', ReadWrite);
    DefineInt(TaSchleichen, 'SCHL', ReadWrite);
    DefineInt(TaSelbstbeherrschung, 'SBHR', ReadWrite);
    DefineInt(TaSinnenschaerfe, 'SINN', ReadWrite);
    DefineInt(TaTanzen, 'TANZ', ReadWrite);
    DefineInt(TaTaschendiebstahl, 'DIEB', ReadWrite);
    DefineInt(TaZechen, 'ZECH', ReadWrite);
    DefineInt(TaFallenstellen, 'FALS', ReadWrite);
    DefineInt(TaFischen, 'FISH', ReadWrite);
    DefineInt(TaOrientierung, 'ORIE', ReadWrite);
    DefineInt(TaWildnisleben, 'WILD', ReadWrite);
    DefineInt(TaMagiekunde, 'MAGK', ReadWrite);
    DefineInt(TaPflanzenkunde, 'PFLA', ReadWrite);
    DefineInt(TaTierkunde, 'TIER', ReadWrite);
    DefineInt(TaSchaetzen, 'SCHT', ReadWrite);
    DefineInt(TaSprachkunde, 'SPRC', ReadWrite);
    DefineInt(TaAlchimie, 'ALCH', ReadWrite);
    DefineInt(TaBogenbau, 'BOGB', ReadWrite);
    DefineInt(TaGrobschmied, 'GSHM', ReadWrite);
    DefineInt(TaHeilkundeWunden, 'HLWU', ReadWrite);
    DefineInt(TaSchloesser, 'SCHL', ReadWrite);
    DefineInt(TaMusizieren, 'MUSI', ReadWrite);
    DefineInt(TaFalschspiel, 'FLSS', ReadWrite);
    DefineInt(TaLederarbeiten, 'LEDR', ReadWrite);
    DefineInt(TaBetoeren, 'BTOR', ReadWrite);
    DefineInt(TaUeberreden, 'UEBR', ReadWrite);
    DefineInt(TaMenschenkenntnis, 'MSHK', ReadWrite);
    
    DefineInt(ZaAdlerauge, 'ADLR', ReadWrite);
    DefineInt(ZaAnalys, 'ANLS', ReadWrite);
    DefineInt(ZaExposami, 'EXPO', ReadWrite);
    DefineInt(ZaOdemArcanum, 'ODEM', ReadWrite);
    DefineInt(ZaDuplicatus, 'DUPL', ReadWrite);
    DefineInt(ZaBlitz, 'BLTZ', ReadWrite);
    DefineInt(ZaFulminictus, 'FULM', ReadWrite);
    DefineInt(ZaIgnifaxius, 'IGFX', ReadWrite);
    DefineInt(ZaIgnisphaero, 'IGSP', ReadWrite);
    DefineInt(ZaKulminatio, 'KULM', ReadWrite);
    DefineInt(ZaPlumbumbaraum, 'PLUM', ReadWrite);
    DefineInt(ZaAttributo, 'ATBO', ReadWrite);
    DefineInt(ZaBeherrschungBrechen, 'BHRB', ReadWrite);
    DefineInt(ZaEinflussBannen, 'EFBN', ReadWrite);
    DefineInt(ZaVerwandlungBeenden, 'VWBN', ReadWrite);
    DefineInt(ZaGardianum, 'GRDM', ReadWrite);
    DefineInt(ZaTempusStatis, 'TMPS', ReadWrite);
    DefineInt(ZaBannbaladin, 'BBLD', ReadWrite);
    DefineInt(ZaHalluzination, 'HALZ', ReadWrite);
    DefineInt(ZaHorriphobus, 'HORB', ReadWrite);
    DefineInt(ZaSanftmut, 'SFTM', ReadWrite);
    DefineInt(ZaSomnigravis, 'SMGR', ReadWrite);
    DefineInt(ZaElementarerDiener, 'ELDR', ReadWrite);
    DefineInt(ZaTatzeSchwinge, 'TATZ', ReadWrite);
    DefineInt(ZaAxxeleratus, 'AXXL', ReadWrite);
    DefineInt(ZaForamen, 'FRAM', ReadWrite);
    DefineInt(ZaTransversalis, 'TRVS', ReadWrite);
    DefineInt(ZaMotoricus, 'MTRS', ReadWrite);
    DefineInt(ZaBalsam, 'BLSM', ReadWrite);
    DefineInt(ZaRuheKoerper, 'RKRP', ReadWrite);
    DefineInt(ZaAdlerschwinge, 'ADLS', ReadWrite);
    DefineInt(ZaArmatrutz, 'ARTZ', ReadWrite);
    DefineInt(ZaParalysis, 'PRLS', ReadWrite);
    DefineInt(ZaUnsichtbarerJaeger, 'USBJ', ReadWrite);
    DefineInt(ZaVisibility, 'VISB', ReadWrite);
    DefineInt(ZaStandfestKatzengleich, 'SKTG', ReadWrite);
    DefineInt(ZaApplicatus, 'APLS', ReadWrite);
    DefineInt(ZaClaudibus, 'CLBS', ReadWrite);
    DefineInt(ZaFlimFlam, 'FLFL', ReadWrite);
    DefineInt(ZaFortifex, 'FRTX', ReadWrite);
    DefineInt(ZaSilentium, 'SLTM', ReadWrite);
    DefineInt(ZaEisenrost, 'ESRS', ReadWrite);
    
    DefineInt(LEmax, 'LEMX', ReadWrite);
    DefineInt(AUmax, 'AUMX', ReadWrite);
    DefineInt(AEmax, 'AEMX', ReadWrite);
    DefineInt(ATbasis, 'ATBS', ReadWrite);
    DefineInt(PAbasis, 'PABS', ReadWrite);
    DefineInt(FKbasis, 'FKBS', ReadWrite);
    DefineInt(MR, 'MR__', ReadWrite);
    DefineInt(LE, 'LE__', ReadWrite);
    DefineInt(AU, 'AU__', ReadWrite);
    DefineInt(AE, 'AE__', ReadWrite);
    DefineInt(AT, 'AT__', ReadWrite);
    DefineInt(PA, 'PA__', ReadWrite);
    DefineInt(BE, 'BE__', ReadWrite);
    
    DefineString(SetupGroups, 'SPGR', ReadWrite);
    DefineString(TargetSize, 'TGTS', ReadOnly);
    DefineString(MapMarkerResource, 'MMRS', ReadWrite);
    DefineString(VisibilityType, 'VIST', ReadOnly);
    DefineString(ScriptPreset, 'SCRP', ReadWrite);
    DefineString(ScriptOverride, 'SCRO', ReadWrite);
}
