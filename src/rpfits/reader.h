/** \file reader.h
 *  \brief Definitions, macros and functions to make reading from RPFITS files
 *         more convenient in this library.
 *
 * ATCA Training Library: reader.h
 * (C) Jamie Stevens CSIRO 2020
 *
 * This header file defines useful macros and functions to make it easier to
 * read from RPFITS files.
 *
 */

#pragma once
#include "atrpfits.h"

/* RPFITS commands numerical definitions to make the code more readable */
/*! \def JSTAT_OPENFILE
 *  \brief Magic number used to open an RPFITS file.
 *
 * This parameter is given as this_jstat in calls to rpfitsin_ to get the
 * RPFITS library to open the file specified in names_.file.
 */
#define JSTAT_OPENFILE -3
/*! \def JSTAT_OPENFILE_READHEADER
 *  \brief Magic number used to open a file and immediately read the
 *         first header in that file.
 *
 * This parameter is given as this_jstat in calls to rpfitsin_ to get the
 * RPFITS library to open the file specified in names_.file and read the
 * first header.
 */
#define JSTAT_OPENFILE_READHEADER -2
/*! \def JSTAT_READNEXTHEADER
 *  \brief Magic number used to read the next header in the currently
 *         open file.
 *
 * This parameter is given as this_jstat in calls to rpfitsin_ to get the
 * RPFITS library to skip all further data until the next header is encountered
 * and read in.
 */
#define JSTAT_READNEXTHEADER -1
/*! \def JSTAT_READDATA
 *  \brief Magic number used to read some data in the currently open file.
 *
 * This parameter is given as this_jstat in calls to rpfitsin_ to get the
 * RPFITS library to read a cycle of data from the currently open file.
 */
#define JSTAT_READDATA 0
/*! \def JSTAT_CLOSEFILE
 *  \brief Magic number used to close the currently open file.
 *
 * This parameter is given as this_jstat in calls to rpfitsin_ to get the
 * RPFITS library to close the currently open file.
 */
#define JSTAT_CLOSEFILE 1
/*! \def JSTAT_SKIPTOEND
 *  \brief Magic number used to move the internal file pointer to the
 *         end of the currently open file.
 *
 * This parameter is given as this_jstat in calls to rpfitsin_ to get the
 * RPFITS library to move to the end of the currently open file. This would
 * be useful if you wanted to append data to an existing RPFITS file.
 */
#define JSTAT_SKIPTOEND 2

/* RPFITS return codes definitions to make the code more readable */
/*! \def JSTAT_UNSUCCESSFUL
 *  \brief RPFITS return code meaning the last operation failed.
 */
#define JSTAT_UNSUCCESSFUL -1
/*! \def JSTAT_SUCCESSFUL
 *  \brief RPFITS return code meaning the last operation succeded.
 */
#define JSTAT_SUCCESSFUL 0
/*! \def JSTAT_HEADERNOTDATA
 *  \brief RPFITS return code meaning that data was expected but a header
 *         was encountered while reading from the file.
 */
#define JSTAT_HEADERNOTDATA 1
/*! \def JSTAT_ENDOFSCAN
 *  \brief RPFITS return code meaning that the end of a scan was reached.
 *         *This is not necessarily an error*.
 */
#define JSTAT_ENDOFSCAN 2
/*! \def JSTAT_ENDOFFILE
 *  \brief RPFITS return code meaning that the end of the file was reached.
 */
#define JSTAT_ENDOFFILE 3
/*! \def JSTAT_FGTABLE
 *  \brief RPFITS return code meaning that the RPFITS FG table was found.
 */
#define JSTAT_FGTABLE 4
/*! \def JSTAT_ILLEGALDATA
 *  \brief RPFITS return code meaning that the reader encountered some data
 *         it could not process.
 */
#define JSTAT_ILLEGALDATA 5

/* Macros to get information. */
/*! \def SOURCENAME
 *  \brief Get the name of source \a s from the RPFITS variables.
 *  \param s the number of the source (starts at 0)
 *  \return a pointer to the start of the source name string
 *
 * A convenience macro to determine the pointer location to obtain the
 * name of the source with number \a s from the RPFITS variables, after
 * reading in the header.
 */
#define SOURCENAME(s) names_.su_name + (s) * SOURCE_LENGTH
/*! \def RIGHTASCENSION
 *  \brief Get the right ascension of source \a s from the RPFITS variables.
 *  \param s the number of the source (starts at 0)
 *  \return right ascension of the source in radians [double]
 *
 * A convenience macro to retrieve the Right Ascension of source with
 * number \a s from the RPFITS variables, after reading in the header.
 */
#define RIGHTASCENSION(s) doubles_.su_ra[s]
/*! \def DECLINATION
 *  \brief Get the right ascension of source \a s from the RPFITS variables.
 *  \param s the number of the source (starts at 0)
 *  \return right ascension of the source in radians [double]
 *
 * A convenience macro to retrieve the Declination of source with
 * number \a s from the RPFITS variables, after reading in the header.
 */
#define DECLINATION(s) doubles_.su_dec[s]
/*! \def CALCODE
 *  \brief Get the calibrator code of source \a s from the RPFITS variables.
 *  \param s the number of the source (starts at 0)
 *  \return a pointer to the start of the calibrator code string
 *
 * A convenience macro to determine the pointer location to obtain the
 * calibrator code of the source with number \a s from the RPFITS variables,
 * after reading in the header.
 */
#define CALCODE(s) names_.su_cal + (s) * CALCODE_LENGTH
/*! \def FREQUENCYMHZ
 *  \brief Get the central frequency of IF \a f from the RPFITS variables.
 *  \param f the number of the IF (starts at 0)
 *  \return central frequency of the IF in MHz [double]
 *
 * A convenience macro to retrieve the frequency at the centre of the IF with
 * number \a f from the RPFITS variables, after reading in the header.
 */
#define FREQUENCYMHZ(f) doubles_.if_freq[f] / 1e6
/*! \def BANDWIDTHMHZ
 *  \brief Get the total bandwidth of IF \a f from the RPFITS variables.
 *  \param f the number of the IF (starts at 0)
 *  \return total bandwidth of the IF in MHz [double]
 *
 * A convenience macro to retrieve the total bandwidth of the IF with
 * number \a f from the RPFITS variables, after reading in the header.
 */
#define BANDWIDTHMHZ(f) doubles_.if_bw[f] / 1e6
/*! \def NCHANNELS
 *  \brief Get the number of channels in IF \a f from the RPFITS variables.
 *  \param f the number of the IF (starts at 0)
 *  \return number of channels in the IF [int]
 *
 * A convenience macro to retrieve the number of channels in the IF with
 * number \a f from the RPFITS variables, after reading in the header.
 */
#define NCHANNELS(f) if_.if_nfreq[f]
/*! \def NSTOKES
 *  \brief Get the number of Stokes parameters in IF \a f from the RPFITS variables.
 *  \param f the number of the IF (starts at 0)
 *  \return number of Stokes parameters in the IF [int]
 *
 * A convenience macro to retrieve the number of Stokes parameters in the IF with
 * number \a f from the RPFITS variables, after reading in the header.
 */
#define NSTOKES(f) if_.if_nstok[f]
/*! \def SIDEBAND
 *  \brief Get a flag representing the sideband of IF \a f from the RPFITS variables.
 *  \param f the number of the IF (starts at 0)
 *  \return either +1 for USB IF, or -1 for LSB IF [int]
 *
 * A convenience macro to retrieve the sideband inversion factor for the IF with
 * number \a f from the RPFITS variables, after reading in the header.
 */
#define SIDEBAND(f) if_.if_invert[f]
/*! \def CHAIN
 *  \brief Get the number of the RF signal chain used to produce the IF \a f
 *         from the RPFITS variables
 *  \param f the number of the IF (starts at 0)
 *  \return the RF signal chain number, which starts at 1 [int]
 *
 * A convenience macro to retrieve the RF signal chain number used to produce the
 * IF with number \a f from the RPFITS variables, after reading in the header.
 */
#define CHAIN(f) if_.if_chain[f]
/*! \def LABEL
 *  \brief Get the assigned number of the IF \a f from the RPFITS variables
 *  \param f the number of the IF (starts at 0)
 *  \return the assigned IF number, which starts at 1 [int]
 *
 * A convenience macro to retrieve the assigned IF number for the
 * IF with number \a f from the RPFITS variables, after reading in the header.
 */
#define LABEL(f) if_.if_num[f]
/*! \def CSTOKES
 *  \brief Get the name for the \a s Stokes parameter in IF \a f from the
 *         RPFITS variables
 *  \param f the number of the IF (starts at 0)
 *  \param s the number of the Stokes parameter (starts at 0)
 *  \return a pointer to the start of the Stokes parameter string
 *
 * A convenience macro to determine the pointer location to obtain the
 * Stokes parameter name for the parameter with number \a s in the IF with
 * number \a f from the RPFITS variables, after reading in the header.
 */
#define CSTOKES(f,s) names_.if_cstok + (s * 2 + f * 4 * 2)
/*! \def ANTNUM
 *  \brief Get the assigned number of the antenna \a a from the RPFITS variables
 *  \param a the number of the antenna (starts at 0)
 *  \return the assigned number of the antenna, which starts at 1 [int]
 *
 * A convenience macro to retrieve the assigned antenna number for the antenna
 * with number \a a from the RPFITS variables, after reading in the header.
 */
#define ANTNUM(a) anten_.ant_num[a]
/*! \def ANTSTATION
 *  \brief Get the name of the station that antenna \a a is connected to from
 *         the RPFITS variables
 *  \param a the number of the antenna (starts at 0)
 *  \return a pointer to the start of the station name string
 *
 * A convenience macro to determine the pointer location to obtain the station
 * name that the antenna with number \a a is connected to from the RPFITS variables,
 * after reading in the header.
 */
#define ANTSTATION(a) names_.sta + (a * 8)
/*! \def ANTX
 *  \brief Get the X-coordinate of the antenna \a a from the RPFITS variables
 *  \param a the number of the antenna (starts at 0)
 *  \return the X-coordinate of the position of antenna \a a on the WGS84
 *          Cartesian plane, in m [double]
 *
 * A convenience macro to retrieve the position of the antenna with number \a a
 * from the RPFITS variables, after reading in the header. To get the full position
 * this macro needs to be used along with two others.
 */
#define ANTX(a) doubles_.x[a]
/*! \def ANTY
 *  \brief Get the Y-coordinate of the antenna \a a from the RPFITS variables
 *  \param a the number of the antenna (starts at 0)
 *  \return the Y-coordinate of the position of antenna \a a on the WGS84
 *          Cartesian plane, in m [double]
 *
 * A convenience macro to retrieve the position of the antenna with number \a a
 * from the RPFITS variables, after reading in the header. To get the full position
 * this macro needs to be used along with two others.
 */
#define ANTY(a) doubles_.y[a]
/*! \def ANTZ
 *  \brief Get the Z-coordinate of the antenna \a a from the RPFITS variables
 *  \param a the number of the antenna (starts at 0)
 *  \return the Z-coordinate of the position of antenna \a a on the WGS84
 *          Cartesian plane, in m [double]
 *
 * A convenience macro to retrieve the position of the antenna with number \a a
 * from the RPFITS variables, after reading in the header. To get the full position
 * this macro needs to be used along with two others.
 */
#define ANTZ(a) doubles_.z[a]

/* These macros allow for easier interrogation of the SYSCAL table. */
/*! \def SYSCAL_NUM_ANTS
 *  \brief Get the number of antennas represented in the SYSCAL table from the
 *         RPFITS variables
 *  \return the number of antennas in the SYSCAL table [int]
 *
 * A convenience macro to retrieve the number of antennas that the SYSCAL table
 * contains information about. This will only return a non-zero number if the SYSCAL
 * table has just been read. This macro is here only to make the code more readable.
 */
#define SYSCAL_NUM_ANTS   sc_.sc_ant
/*! \def SYSCAL_NUM_IFS
 *  \brief Get the number of IFs represented in the SYSCAL table from the
 *         RPFITS variables
 *  \return the number of IFs in the SYSCAL table [int]
 *
 * A convenience macro to retrieve the number of IFs that the SYSCAL table
 * contains information about. This will only return a non-zero number if the SYSCAL
 * table has just been read. This macro is here only to make the code more readable.
 */
#define SYSCAL_NUM_IFS    sc_.sc_if
/*! \def SYSCAL_NUM_PARAMS
 *  \brief Get the number of parameters represented in the SYSCAL table from the
 *         RPFITS variables
 *  \return the number of parameters in the SYSCAL table [int]
 *
 * A convenience macro to retrieve the number of parameters that the SYSCAL table
 * contains information about. This will only return a non-zero number if the SYSCAL
 * table has just been read. This macro is here only to make the code more readable.
 * A SYSCAL parameter is something like Tsys, or parallactic angle etc.
 */
#define SYSCAL_NUM_PARAMS sc_.sc_q
/*! \def SYSCAL_GRP
 *  \brief Get the array index for the SYSCAL parameters pertaining to antenna
 *         \a a and IF \a i
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the array index at the start of the SYSCAL record for antenna \a a
 *          and IF \a i
 *
 * Convenience macro to index the SYSCAL array from the RPFITS variables. Since
 * the SYSCAL array is a single flat array, the index depends on how many IFs are
 * present, how many parameters are presented, and which antenna and IF is desired.
 * This macro is here only to make the code more readable.
 */
#define SYSCAL_GRP(a, i) (a * (SYSCAL_NUM_IFS * SYSCAL_NUM_PARAMS) + i * SYSCAL_NUM_PARAMS)
/*! \def SYSCAL_PARAM
 *  \brief Get the parameter value for the SYCAL parameter with index \a o pertaining 
 *         to antenna \a a and IF \a i
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \param o the number of the parameter (starts at 0)
 *  \return the value of SYSCAL parameter with index \a o for antenna \a a and IF \a i [float]
 *  
 * Convenience macro to return SYSCAL parameter values from the RPFITS variables.
 * This macro is here only to make the code a lot more readable.
 */
#define SYSCAL_PARAM(a, i, o) (sc_.sc_cal[SYSCAL_GRP(a, i) + o])
/*! \def SYSCAL_ANT
 *  \brief Get the assigned antenna number for the SYSCAL parameter pertaining to
 *         antenna \a a and IF \a i
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the assigned antenna number (starts at 1) [int]
 *
 * Convenience macro to return the assigned antenna number for a particular SYSCAL entry.
 */
#define SYSCAL_ANT(a, i) (int)SYSCAL_PARAM(a, i, 0)
/*! \def SYSCAL_IF
 *  \brief Get the assigned IF number for the SYSCAL parameter pertaining to
 *         antenna \a a and IF \a i
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the assigned IF number (starts at 1) [int]
 *
 * Convenience macro to return the assigned IF number for a particular SYSCAL entry.
 * This macro will only return a valid value if the SYSCAL table has just been read.
 */
#define SYSCAL_IF(a, i) (int)SYSCAL_PARAM(a, i, 1)
/*! \def SYSCAL_XYPHASE
 *  \brief Get the phase between the X and Y polarisations from the SYSCAL parameter 
 *         pertaining to antenna \a a and IF \a i
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the phase between the X and Y polarisations, as measured by the correlator,
 *          for antenna \a a in IF \a i, in degrees [float]
 *
 * Convenience macro to return the correlator-measured XY-phase for a specified
 * antenna \a a and IF \a i.
 * This macro will only return a valid value if the SYSCAL table has just been read.
 */
#define SYSCAL_XYPHASE(a, i) SYSCAL_PARAM(a, i, 2)
/*! \def SYSCAL_TSYS_X
 *  \brief Get the system temperature of the X polarisation from the SYSCAL parameter 
 *         pertaining to antenna \a a and IF \a i
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the system temperature of the X polarisation, as measured by the correlator,
 *          for antenna \a a in IF \a i, in Kelvin [float]
 *
 * Convenience macro to return the correlator-measured X-pol receiver system temperature
 * for a specified antenna \a a and IF \a i.
 * This macro will only return a valid value if the SYSCAL table has just been read.
 */
#define SYSCAL_TSYS_X(a, i) fabsf(SYSCAL_PARAM(a, i, 3))
/*! \def SYSCAL_TSYS_Y
 *  \brief Get the system temperature of the Y polarisation from the SYSCAL parameter 
 *         pertaining to antenna \a a and IF \a i
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the system temperature of the Y polarisation, as measured by the correlator,
 *          for antenna \a a in IF \a i, in Kelvin [float]
 *
 * Convenience macro to return the correlator-measured Y-pol receiver system temperature
 * for a specified antenna \a a and IF \a i.
 * This macro will only return a valid value if the SYSCAL table has just been read.
 */
#define SYSCAL_TSYS_Y(a, i) fabsf(SYSCAL_PARAM(a, i, 4))
/*! \def SYSCAL_TSYS_X_APPLIED
 *  \brief Get a flag of whether the X-pol Tsys has been applied to the data pertaining
 *         to antenna \a a and IF \a i
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return an indication of whether the Tsys has been applied to the X-pol data
 *          (SYSCAL_TSYS_APPLIED) or not (SYSCAL_TSYS_NOT_APPLIED)
 *
 * Convenience macro to return a flag indicating if the correlator scaled the correlated
 * data according to the measured system temperature for the specified antenna \a a and
 * IF \a i. This macro will only return a valid value if the SYSCAL table has just been read.
 */
#define SYSCAL_TSYS_X_APPLIED(a, i) (SYSCAL_PARAM(a, i, 3) < 0) ? SYSCAL_TSYS_NOT_APPLIED : SYSCAL_TSYS_APPLIED
/*! \def SYSCAL_TSYS_Y_APPLIED
 *  \brief Get a flag of whether the Y-pol Tsys has been applied to the data pertaining
 *         to antenna \a a and IF \a i
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return an indication of whether the Tsys has been applied to the Y-pol data
 *          (SYSCAL_TSYS_APPLIED) or not (SYSCAL_TSYS_NOT_APPLIED)
 *
 * Convenience macro to return a flag indicating if the correlator scaled the correlated
 * data according to the measured system temperature for the specified antenna \a a and
 * IF \a i. This macro will only return a valid value if the SYSCAL table has just been read.
 */
#define SYSCAL_TSYS_Y_APPLIED(a, i) (SYSCAL_PARAM(a, i, 4) < 0) ? SYSCAL_TSYS_NOT_APPLIED : SYSCAL_TSYS_APPLIED
/*! \def SYSCAL_GTP_X
 *  \brief Get the gated total power (GTP) calculated by the correlator pertaining
 *         to antenna \a a and IF \a i for the X polarisation
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the correlator-measured X-pol GTP
 *
 * Convenience macro to return the X-pol gated total power for the specified antenna \a a and
 * IF \a i. This macro will only return a valid value if the SYSCAL table has just been read.
 */
#define SYSCAL_GTP_X(a, i) SYSCAL_PARAM(a, i, 5)
/*! \def SYSCAL_SDO_X
 *  \brief Get the synchronously-demodulated output (SDO) calculated by the correlator 
 *         pertaining to antenna \a a and IF \a i for the X polarisation
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the correlator-measured X-pol SDO
 *
 * Convenience macro to return the X-pol synchronously-demodulated output level for the specified 
 * antenna \a a and IF \a i. This macro will only return a valid value if the SYSCAL table 
 * has just been read.
 */
#define SYSCAL_SDO_X(a, i) SYSCAL_PARAM(a, i, 6)
/*! \def SYSCAL_CALJY_X
 *  \brief Get the assumed strength of the noise diode pertaining to antenna \a a and 
 *         IF \a i for the X polarisation
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the correlator-assumed X-pol noise diode amplitude [Jy]
 *
 * Convenience macro to return the X-pol amplitude of the noise diode, in Jy, that the correlator
 * was assuming when calculating the system temperatures for this cycle, for the specified
 * antenna \a a and IF \a i. This macro will only return a valid value if the SYSCAL table 
 * has just been read.
 */
#define SYSCAL_CALJY_X(a, i) SYSCAL_PARAM(a, i, 7)
/*! \def SYSCAL_GTP_Y
 *  \brief Get the gated total power (GTP) calculated by the correlator pertaining
 *         to antenna \a a and IF \a i for the Y polarisation
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the correlator-measured Y-pol GTP
 *
 * Convenience macro to return the Y-pol gated total power for the specified antenna \a a and
 * IF \a i. This macro will only return a valid value if the SYSCAL table has just been read.
 */
#define SYSCAL_GTP_Y(a, i) SYSCAL_PARAM(a, i, 8)
/*! \def SYSCAL_SDO_Y
 *  \brief Get the synchronously-demodulated output (SDO) calculated by the correlator 
 *         pertaining to antenna \a a and IF \a i for the Y polarisation
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the correlator-measured Y-pol SDO
 *
 * Convenience macro to return the Y-pol synchronously-demodulated output level for the specified 
 * antenna \a a and IF \a i. This macro will only return a valid value if the SYSCAL table 
 * has just been read.
 */
#define SYSCAL_SDO_Y(a, i) SYSCAL_PARAM(a, i, 9)
/*! \def SYSCAL_CALJY_Y
 *  \brief Get the assumed strength of the noise diode pertaining to antenna \a a and 
 *         IF \a i for the Y polarisation
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the correlator-assumed Y-pol noise diode amplitude [Jy]
 *
 * Convenience macro to return the Y-pol amplitude of the noise diode, in Jy, that the correlator
 * was assuming when calculating the system temperatures for this cycle, for the specified
 * antenna \a a and IF \a i. This macro will only return a valid value if the SYSCAL table 
 * has just been read.
 */
#define SYSCAL_CALJY_Y(a, i) SYSCAL_PARAM(a, i, 10)
/*! \def SYSCAL_PARANGLE
 *  \brief Get the parallactic angle of the cycle with regards the antenna \a a and IF
 *         \a i
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the parallactic angle [deg]
 *
 * Convenience macro to return the parallactic angle calculated by the correlator for the
 * current cycle, for the specified antenna \a a and IF \a i. This macro will only return
 * a valid value if the SYSCAL table has just been read.
 */
#define SYSCAL_PARANGLE(a, i) SYSCAL_PARAM(a, i, 11)
/*! \def SYSCAL_FLAG_BAD
 *  \brief Get an indicator of whether this data has been flagged bad for the antenna
 *         \a a and IF \a i
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return a bitwise OR indication that the data in this cycle is bad for the reason:
 *          - 1 the antenna was not on source
 *          - 2 the correlator flagged it for pol X
 *          - 4 the correlator flagged it for pol Y
 *
 * Convenience macro to return the flagging status of the
 * current cycle, for the specified antenna \a a and IF \a i. This macro will only return
 * a valid value if the SYSCAL table has just been read.
 */
#define SYSCAL_FLAG_BAD(a, i) (int)SYSCAL_PARAM(a, i, 12)
/*! \def SYSCAL_XYAMP
 *  \brief Get the XY-amplitude measured by the correlator for this the cycle pertaining
 *         to the antenna \a a and IF \a i
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the XY-amplitude [Jy]
 *
 * Convenience macro to return the XY-amplitude measured by the correlator for the
 * current cycle, for the specified antenna \a a and IF \a i. This macro will only return
 * a valid value if the SYSCAL table has just been read.
 */
#define SYSCAL_XYAMP(a, i) SYSCAL_PARAM(a, i, 13)
/*! \def SYSCAL_TRACKERR_MAX
 *  \brief Get the maximum tracking error of antenna \a a (and IF \a i, but this should
 *         of course be invariant) observed during this cycle
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the maximum tracking error [arcsec]
 *
 * Convenience macro to return the maximum tracking error observed during
 * current cycle, for the specified antenna \a a and IF \a i. This macro will only return
 * a valid value if the SYSCAL table has just been read.
 */
#define SYSCAL_TRACKERR_MAX(a, i) SYSCAL_PARAM(a, i, 14)
/*! \def SYSCAL_TRACKERR_RMS
 *  \brief Get the RMS tracking error of antenna \a a (and IF \a i, but this should
 *         of course be invariant) observed during this cycle
 *  \param a the number of the antenna (starts at 0)
 *  \param i the number of the IF (starts at 0)
 *  \return the RMS tracking error [arcsec]
 *
 * Convenience macro to return the RMS tracking error observed during
 * current cycle, for the specified antenna \a a and IF \a i. This macro will only return
 * a valid value if the SYSCAL table has just been read.
 */
#define SYSCAL_TRACKERR_RMS(a, i) SYSCAL_PARAM(a, i, 15)
/* These next macros will work if SYSCAL_ADDITIONAL_CHECK returns 0. */
/*! \def SYSCAL_ADDITIONAL_CHECK
 *  \brief Return a check flag for whether the SYSCAL record just read corresponds
 *         to an RPFITS additional SYSCAL record, which carries different information
 *  \return 0 if the SYSCAL record just read matches the additional record, or not
 *          0 otherwise
 *
 * RPFITS SYSCAL records can either be per-antenna and per-IF, or an additional
 * record which contains information about site-based parameters. This check macro
 * can be used to determine which type of record was just read; if this macro returns 0
 * it must be the additional record. If that is the case, all the SYSCAL_ADDITIONAL_*
 * convenience macros can be used, otherwise the SYSCAL_* macros should be used.
 */
#define SYSCAL_ADDITIONAL_CHECK (int)SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 0)
#define SYSCAL_ADDITIONAL_TEMP SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 1)
#define SYSCAL_ADDITIONAL_AIRPRESS SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 2)
#define SYSCAL_ADDITIONAL_HUMI SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 3)
#define SYSCAL_ADDITIONAL_WINDSPEED SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 4)
#define SYSCAL_ADDITIONAL_WINDDIR SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 5)
#define SYSCAL_ADDITIONAL_WEATHERFLAG (int)SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 6)
#define SYSCAL_ADDITIONAL_RAIN SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 7)
#define SYSCAL_ADDITIONAL_SEEMON_PHASE SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 8)
#define SYSCAL_ADDITIONAL_SEEMON_RMS SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 9)
#define SYSCAL_ADDITIONAL_SEEMON_FLAG (int)SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 10)

// Return values from the read routines.
#define READER_EXHAUSTED 0
#define READER_DATA_AVAILABLE 1
#define READER_HEADER_AVAILABLE 2

int size_of_vis(void);
int size_of_if_vis(int if_no);
int max_size_of_vis(void);
int open_rpfits_file(char *filename);
int close_rpfits_file(void);
void string_copy(char *start, int length, char *dest);
void get_card_value(char *header_name, char *value, int value_maxlength);
int read_scan_header(struct scan_header_data *scan_header_data);
struct cycle_data* prepare_new_cycle_data(void);
struct scan_data* prepare_new_scan_data(void);
struct cycle_data* scan_add_cycle(struct scan_data *scan_data);
void free_scan_header_data(struct scan_header_data *scan_header_data);
void free_cycle_data(struct cycle_data *cycle_data);
void free_scan_data(struct scan_data *scan_data);
int read_cycle_data(struct scan_header_data *scan_header_data,
		    struct cycle_data *cycle_data);
//int generate_rpfits_index(struct rpfits_index *rpfits_index);
