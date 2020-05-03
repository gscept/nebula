#ifndef BENCHMARKING_DBATTRS_H
#define BENCHMARKING_DBATTRS_H
//------------------------------------------------------------------------------
/**
    @class Benchmarking::DbAttrs
    
    Attributes for the database benchmarks.
    
    (C) 2006 Radon Labs GmbH
*/
#include "attr/attribute.h"

#ifdef IN
#undef IN
#endif

//------------------------------------------------------------------------------
namespace Attr
{
    // this is taken from the DSA NPC table
    DeclareGuid(Guid, 'GUID', ReadOnly);
    DeclareString(Level, 'LEVL', ReadWrite);
    DeclareString(Id, 'ID__', ReadOnly);
    DeclareString(Graphics, 'GFX_', ReadOnly);
    DeclareBool(Rot180, 'R180', ReadOnly);
    DeclareString(CharacterSet, 'CHST', ReadOnly);
    DeclareString(AnimSet, 'ANST', ReadOnly);
    DeclareString(SoundSet, 'SDST', ReadOnly);
    DeclareString(Placeholder, 'PLCH', ReadOnly);
    DeclareString(Name, 'NAME', ReadWrite);
    DeclareString(EntityGroup, 'EGRP', ReadWrite);
    DeclareString(Faction, 'FCTN', ReadOnly);
    DeclareString(Behaviour, 'BHVR', ReadOnly);
    DeclareFloat(MaxVelocity, 'MVEL', ReadOnly);
    DeclareString(LookatText, 'LOKT', ReadWrite);
    DeclareBool(NewHero, 'NWHR', ReadWrite);
    DeclareFloat(AggroRadius, 'AGRD', ReadOnly);
    DeclareString(SetupStorage, 'STST', ReadWrite);
    DeclareString(SetupEquipment, 'STEQ', ReadWrite);
    DeclareString(InventoryType, 'INVT', ReadWrite);
    
    DeclareInt(MU, 'MU__', ReadWrite);
    DeclareInt(KL, 'KL__', ReadWrite);
    DeclareInt(IN, 'IN__', ReadWrite);
    DeclareInt(CH, 'CH__', ReadWrite);
    DeclareInt(FF, 'FF__', ReadWrite);
    DeclareInt(GE, 'GE__', ReadWrite);
    DeclareInt(KO, 'KO__', ReadWrite);
    DeclareInt(KK, 'KK__', ReadWrite);
    
    DeclareInt(TaArmbrust, 'ARMB', ReadWrite);
    DeclareInt(TaATArmbrust, 'AARM', ReadWrite);
    DeclareInt(TaPAArmbrust, 'PARM', ReadWrite);
    DeclareInt(TaDolche, 'DOLC', ReadWrite);
    DeclareInt(TaATDolche, 'ADOL', ReadWrite);
    DeclareInt(TaPADolche, 'PDOL', ReadWrite);
    DeclareInt(TaFechtwaffen, 'FECH', ReadWrite);
    DeclareInt(TaATFechtwaffen, 'AFEC', ReadWrite);
    DeclareInt(TaPAFechtwaffen, 'PFEC', ReadWrite);
    DeclareInt(TaHiebwaffen, 'HIEB', ReadWrite);
    DeclareInt(TaATHiebwaffen, 'AHIE', ReadWrite);
    DeclareInt(TaPAHiebwaffen, 'PHIE', ReadWrite);
    DeclareInt(TaInfanteriewaffen, 'INFW', ReadWrite);
    DeclareInt(TaATInfanteriewaffen, 'AINF', ReadWrite);
    DeclareInt(TaPAInfanteriewaffen, 'PINF', ReadWrite);
    DeclareInt(TaKettenwaffen, 'KETT', ReadWrite);
    DeclareInt(TaATKettenwaffen, 'AKET', ReadWrite);
    DeclareInt(TaPAKettenwaffen, 'PKET', ReadWrite);
    DeclareInt(TaSaebel, 'SAEB', ReadWrite);
    DeclareInt(TaATSaebel, 'ASAE', ReadWrite);
    DeclareInt(TaPASaebel, 'PSAE', ReadWrite);
    DeclareInt(TaSchwerter, 'SCHW', ReadWrite);
    DeclareInt(TaATSchwerter, 'ASCH', ReadWrite);
    DeclareInt(TaPASchwerter, 'PSCH', ReadWrite);
    DeclareInt(TaSpeere, 'SPEE', ReadWrite);
    DeclareInt(TaATSpeere, 'ASPE', ReadWrite);
    DeclareInt(TaPASpeere, 'PSPE', ReadWrite);
    DeclareInt(TaStaebe, 'STAE', ReadWrite);
    DeclareInt(TaATStaebe, 'ASTA', ReadWrite);
    DeclareInt(TaPAStaebe, 'PSTA', ReadWrite);
    DeclareInt(TaZwHiebwaffen, 'ZWHB', ReadWrite);
    DeclareInt(TaATZwHiebwaffen, 'AZWH', ReadWrite);
    DeclareInt(TaPAZwHiebwaffen, 'PZWH', ReadWrite);
    DeclareInt(TaZwSchwerter, 'ZWHS', ReadWrite);
    DeclareInt(TaATZwSchwerter, 'AZWS', ReadWrite);
    DeclareInt(TaPAZwSchwerter, 'PZWS', ReadWrite);
    DeclareInt(TaRaufen, 'RAUF', ReadWrite);
    DeclareInt(TaATRaufen, 'ARAU', ReadWrite);
    DeclareInt(TaPARaufen, 'PRAU', ReadWrite);
    DeclareInt(TaBogen, 'BOGN', ReadWrite);
    DeclareInt(TaWurfbeile, 'WRFB', ReadWrite);
    DeclareInt(TaWurfmesser, 'WRFM', ReadWrite);
    DeclareInt(TaWurfspeer, 'WRFS', ReadWrite);

    DeclareInt(TaGaukeleien, 'GAUK', ReadWrite);
    DeclareInt(TaKoerperbeherrschung, 'KBHR', ReadWrite);
    DeclareInt(TaSchleichen, 'SCHL', ReadWrite);
    DeclareInt(TaSelbstbeherrschung, 'SBHR', ReadWrite);
    DeclareInt(TaSinnenschaerfe, 'SINN', ReadWrite);
    DeclareInt(TaTanzen, 'TANZ', ReadWrite);
    DeclareInt(TaTaschendiebstahl, 'DIEB', ReadWrite);
    DeclareInt(TaZechen, 'ZECH', ReadWrite);
    DeclareInt(TaFallenstellen, 'FALS', ReadWrite);
    DeclareInt(TaFischen, 'FISH', ReadWrite);
    DeclareInt(TaOrientierung, 'ORIE', ReadWrite);
    DeclareInt(TaWildnisleben, 'WILD', ReadWrite);
    DeclareInt(TaMagiekunde, 'MAGK', ReadWrite);
    DeclareInt(TaPflanzenkunde, 'PFLA', ReadWrite);
    DeclareInt(TaTierkunde, 'TIER', ReadWrite);
    DeclareInt(TaSchaetzen, 'SCHT', ReadWrite);
    DeclareInt(TaSprachkunde, 'SPRC', ReadWrite);
    DeclareInt(TaAlchimie, 'ALCH', ReadWrite);
    DeclareInt(TaBogenbau, 'BOGB', ReadWrite);
    DeclareInt(TaGrobschmied, 'GSHM', ReadWrite);
    DeclareInt(TaHeilkundeWunden, 'HLWU', ReadWrite);
    DeclareInt(TaSchloesser, 'SCHL', ReadWrite);
    DeclareInt(TaMusizieren, 'MUSI', ReadWrite);
    DeclareInt(TaFalschspiel, 'FLSS', ReadWrite);
    DeclareInt(TaLederarbeiten, 'LEDR', ReadWrite);
    DeclareInt(TaBetoeren, 'BTOR', ReadWrite);
    DeclareInt(TaUeberreden, 'UEBR', ReadWrite);
    DeclareInt(TaMenschenkenntnis, 'MSHK', ReadWrite);
    
    DeclareInt(ZaAdlerauge, 'ADLR', ReadWrite);
    DeclareInt(ZaAnalys, 'ANLS', ReadWrite);
    DeclareInt(ZaExposami, 'EXPO', ReadWrite);
    DeclareInt(ZaOdemArcanum, 'ODEM', ReadWrite);
    DeclareInt(ZaDuplicatus, 'DUPL', ReadWrite);
    DeclareInt(ZaBlitz, 'BLTZ', ReadWrite);
    DeclareInt(ZaFulminictus, 'FULM', ReadWrite);
    DeclareInt(ZaIgnifaxius, 'IGFX', ReadWrite);
    DeclareInt(ZaIgnisphaero, 'IGSP', ReadWrite);
    DeclareInt(ZaKulminatio, 'KULM', ReadWrite);
    DeclareInt(ZaPlumbumbaraum, 'PLUM', ReadWrite);
    DeclareInt(ZaAttributo, 'ATBO', ReadWrite);
    DeclareInt(ZaBeherrschungBrechen, 'BHRB', ReadWrite);
    DeclareInt(ZaEinflussBannen, 'EFBN', ReadWrite);
    DeclareInt(ZaVerwandlungBeenden, 'VWBN', ReadWrite);
    DeclareInt(ZaGardianum, 'GRDM', ReadWrite);
    DeclareInt(ZaTempusStatis, 'TMPS', ReadWrite);
    DeclareInt(ZaBannbaladin, 'BBLD', ReadWrite);
    DeclareInt(ZaHalluzination, 'HALZ', ReadWrite);
    DeclareInt(ZaHorriphobus, 'HORB', ReadWrite);
    DeclareInt(ZaSanftmut, 'SFTM', ReadWrite);
    DeclareInt(ZaSomnigravis, 'SMGR', ReadWrite);
    DeclareInt(ZaElementarerDiener, 'ELDR', ReadWrite);
    DeclareInt(ZaTatzeSchwinge, 'TATZ', ReadWrite);
    DeclareInt(ZaAxxeleratus, 'AXXL', ReadWrite);
    DeclareInt(ZaForamen, 'FRAM', ReadWrite);
    DeclareInt(ZaTransversalis, 'TRVS', ReadWrite);
    DeclareInt(ZaMotoricus, 'MTRS', ReadWrite);
    DeclareInt(ZaBalsam, 'BLSM', ReadWrite);
    DeclareInt(ZaRuheKoerper, 'RKRP', ReadWrite);
    DeclareInt(ZaAdlerschwinge, 'ADLS', ReadWrite);
    DeclareInt(ZaArmatrutz, 'ARTZ', ReadWrite);
    DeclareInt(ZaParalysis, 'PRLS', ReadWrite);
    DeclareInt(ZaUnsichtbarerJaeger, 'USBJ', ReadWrite);
    DeclareInt(ZaVisibility, 'VISB', ReadWrite);
    DeclareInt(ZaStandfestKatzengleich, 'SKTG', ReadWrite);
    DeclareInt(ZaApplicatus, 'APLS', ReadWrite);
    DeclareInt(ZaClaudibus, 'CLBS', ReadWrite);
    DeclareInt(ZaFlimFlam, 'FLFL', ReadWrite);
    DeclareInt(ZaFortifex, 'FRTX', ReadWrite);
    DeclareInt(ZaSilentium, 'SLTM', ReadWrite);
    DeclareInt(ZaEisenrost, 'ESRS', ReadWrite);
    
    DeclareInt(LEmax, 'LEMX', ReadWrite);
    DeclareInt(AUmax, 'AUMX', ReadWrite);
    DeclareInt(AEmax, 'AEMX', ReadWrite);
    DeclareInt(ATbasis, 'ATBS', ReadWrite);
    DeclareInt(PAbasis, 'PABS', ReadWrite);
    DeclareInt(FKbasis, 'FKBS', ReadWrite);
    DeclareInt(MR, 'MR__', ReadWrite);
    DeclareInt(LE, 'LE__', ReadWrite);
    DeclareInt(AU, 'AU__', ReadWrite);
    DeclareInt(AE, 'AE__', ReadWrite);
    DeclareInt(AT, 'AT__', ReadWrite);
    DeclareInt(PA, 'PA__', ReadWrite);
    DeclareInt(BE, 'BE__', ReadWrite);
    
    DeclareString(SetupGroups, 'SPGR', ReadWrite);
    DeclareString(TargetSize, 'TGTS', ReadOnly);
    DeclareString(MapMarkerResource, 'MMRS', ReadWrite);
    DeclareString(VisibilityType, 'VIST', ReadOnly);
    DeclareString(ScriptPreset, 'SCRP', ReadWrite);
    DeclareString(ScriptOverride, 'SCRO', ReadWrite);
}
//------------------------------------------------------------------------------
#endif
