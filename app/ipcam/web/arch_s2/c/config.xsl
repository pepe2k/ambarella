<?xml version="1.0" encoding="GB2312"?><!-- DWXMLSource="cgi-bin/sample.xml" -->
<!--xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl"-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		 xmlns:copyRight="http://xml.sz.luohuedu.net/">
<!-- varables-->
<xsl:variable name="counter" select="0"/>
<xsl:variable name="step" select="1"/>

<!--root-->
<xsl:template match="/">
<HTML><HEAD><TITLE>IP Camera Status</TITLE></HEAD>
<BODY >
<div align="center">
<form id="form1" name="form1" method="post" action="main.cgi">
<xsl:apply-templates select="get/format/vin"/>
<br/>
<xsl:apply-templates select="get/format/vout"/>
<br/>
<xsl:apply-templates select="get/format/venc"/>
<br/>
<xsl:apply-templates select="get/param/main"/>
<br/>
<xsl:apply-templates select="get/param/second"/>
<br/>
<xsl:apply-templates select="get/param/mjpeg"/>
<br/>
<!--input type="submit" name="ipCamSetting" id="submit" value="confirm" /-->
<input type="submit" name="applyModify" id="submit" value="apply" />
</form>
</div>
</BODY>
</HTML>
</xsl:template>
<!--vin-->
<xsl:template match="vin">
<CAPTION>VIN</CAPTION>
<br/>
<TABLE border="1" width="600">
 <tr><td>Mode</td></tr>
<xsl:for-each select="mode">
	<tr><td>
    <xsl:call-template name="radiocontrol1">
    <xsl:with-param name="group_name">Vin_mode</xsl:with-param>
    <xsl:with-param name="radio_name"><xsl:value-of select="."/></xsl:with-param>
    <xsl:with-param name="current1"><xsl:value-of select="@current"/></xsl:with-param>
    </xsl:call-template>
	</td></tr>
</xsl:for-each>
 <tr><td>Frame rate</td></tr>
 <xsl:for-each select="framerate">
	<tr><td>
	<xsl:call-template name="radiocontrol1">
    <xsl:with-param name="group_name">Vin_framerate</xsl:with-param>
    <xsl:with-param name="radio_name"><xsl:value-of select="."/></xsl:with-param>
    <xsl:with-param name="current1"><xsl:value-of select="@current"/></xsl:with-param>
    </xsl:call-template>
	</td></tr> 
</xsl:for-each>
</TABLE>
</xsl:template>

<!--vout-->
<xsl:template match="vout">
<CAPTION>VOUT</CAPTION>
<TABLE border="1" width="600">
 <tr><td>Mode</td></tr>
<xsl:for-each select="mode">
	<tr><td>
    <xsl:call-template name="radiocontrol1">
    <xsl:with-param name="group_name">Vout_mode</xsl:with-param>
    <xsl:with-param name="radio_name"><xsl:value-of select="."/></xsl:with-param>
    <xsl:with-param name="current1"><xsl:value-of select="@current"/></xsl:with-param>
    </xsl:call-template>
	</td></tr>
</xsl:for-each>
</TABLE>
</xsl:template>

<!--venc-->
<xsl:template match="venc">
<CAPTION>VENC</CAPTION>
<TABLE border="1" width="600">
 <tr><td>main stream: 
  <select name="mainstream" id="ms" >
    <xsl:call-template name="option">
    <xsl:with-param name="optionvalue">MJPEG</xsl:with-param>
    <xsl:with-param name="currentvalue"><xsl:value-of select="main/enc_type"/></xsl:with-param>
    </xsl:call-template>
    <xsl:call-template name="option">
    <xsl:with-param name="optionvalue">H264</xsl:with-param>
    <xsl:with-param name="currentvalue"><xsl:value-of select="main/enc_type"/></xsl:with-param>
    </xsl:call-template>
    <xsl:call-template name="option">
    <xsl:with-param name="optionvalue">none</xsl:with-param>
    <xsl:with-param name="currentvalue"><xsl:value-of select="main/enc_type"/></xsl:with-param>
    </xsl:call-template>
  </select> 
 </td></tr>
<xsl:for-each select="main/resolution">
	<tr><td>
    <xsl:call-template name="radiocontrol1">
    <xsl:with-param name="group_name">enc_main_resolution</xsl:with-param>
    <xsl:with-param name="radio_name"><xsl:value-of select="."/></xsl:with-param>
    <xsl:with-param name="current1"><xsl:value-of select="@current"/></xsl:with-param>
    </xsl:call-template>
	</td></tr>
</xsl:for-each>
<tr><td>second stream:
	<select name="secondstream" id="ss" >
    <xsl:call-template name="option">
    <xsl:with-param name="optionvalue">MJPEG</xsl:with-param>
    <xsl:with-param name="currentvalue"><xsl:value-of select="second/enc_type"/></xsl:with-param>
    </xsl:call-template>
    <xsl:call-template name="option">
    <xsl:with-param name="optionvalue">H264</xsl:with-param>
    <xsl:with-param name="currentvalue"><xsl:value-of select="second/enc_type"/></xsl:with-param>
    </xsl:call-template>
    <xsl:call-template name="option">
    <xsl:with-param name="optionvalue">none</xsl:with-param>
    <xsl:with-param name="currentvalue"><xsl:value-of select="second/enc_type"/></xsl:with-param>
    </xsl:call-template>
	</select> 
</td></tr>
<xsl:for-each select="second/resolution">
	<tr><td>
	<xsl:call-template name="radiocontrol1">
    <xsl:with-param name="group_name">enc_second_resolution</xsl:with-param>
    <xsl:with-param name="radio_name"><xsl:value-of select="."/></xsl:with-param>
    <xsl:with-param name="current1"><xsl:value-of select="@current"/></xsl:with-param>
    </xsl:call-template>
	</td></tr>
</xsl:for-each>
</TABLE>
</xsl:template>

<xsl:template name="option">
<xsl:param name="optionvalue"/>
<xsl:param name="currentvalue"/>
<option>
<xsl:if test="$currentvalue=$optionvalue">
<xsl:attribute name="selected"/>
</xsl:if>
<xsl:value-of select="$optionvalue"/>
</option>
</xsl:template>

<!--para-->
<xsl:template match="param/main">
<CAPTION>MAIN</CAPTION>
<xsl:call-template name="encpara">
<xsl:with-param name="stream">main</xsl:with-param>
</xsl:call-template>
</xsl:template>

<xsl:template match="param/second">
<CAPTION>SECOND</CAPTION>
<xsl:call-template name="encpara">
<xsl:with-param name="stream">second</xsl:with-param>
</xsl:call-template>
</xsl:template>

<xsl:template name="encpara">
<xsl:param name="stream"/>
<TABLE border="1" width="600">
	<tr><td><xsl:value-of select="$stream"/></td></tr>
 <tr><td>M : <input type="text" name="{$stream}_M" id="1"><xsl:attribute name="value"><xsl:value-of select="M"/></xsl:attribute></input></td></tr>
 <tr><td>N : <input type="text" name="{$stream}_N" id="2"><xsl:attribute name="value"><xsl:value-of select="N"/></xsl:attribute></input></td></tr>
 <tr><td>idr_interval : <input type="text" name="{$stream}_idr" id="3"><xsl:attribute name="value"><xsl:value-of select="idr_interval"/></xsl:attribute></input></td></tr>
 <tr><td>Gop model</td></tr>
<xsl:for-each select="gop_model">
	<tr><td>
	<xsl:call-template name="radiocontrol1">
    <xsl:with-param name="group_name"><xsl:value-of select="$stream"/>_gop_model</xsl:with-param>
    <xsl:with-param name="radio_name"><xsl:value-of select="."/></xsl:with-param>
    <xsl:with-param name="current1"><xsl:value-of select="@current"/></xsl:with-param>
    </xsl:call-template>
	</td></tr>
</xsl:for-each>
 <tr><td>bitrate</td></tr>
<xsl:for-each select="bitrate_control">
	<tr><td>
	<xsl:call-template name="radiocontrol1">
    <xsl:with-param name="group_name"><xsl:value-of select="$stream"/>_bitrate_mode</xsl:with-param>
    <xsl:with-param name="radio_name"><xsl:value-of select="."/></xsl:with-param>
    <xsl:with-param name="current1"><xsl:value-of select="@current"/></xsl:with-param>
    </xsl:call-template>
	</td></tr>
</xsl:for-each>
<tr><td>vbr_ness : <input type="text" name="{$stream}_vbr_ness" id="4"><xsl:attribute name="value"><xsl:value-of select="vbr_ness"/></xsl:attribute></input></td></tr>
<tr><td>min_vbr_rate_factor : <input type="text" name="{$stream}_min_vbr_rate" id="5"><xsl:attribute name="value"><xsl:value-of select="min_vbr_rate_factor"/></xsl:attribute></input></td></tr>
<tr><td>max_vbr_rate_factor : <input type="text" name="{$stream}_max_vbr_rate" id="6"><xsl:attribute name="value"><xsl:value-of select="max_vbr_rate_factor"/></xsl:attribute></input></td></tr>
<tr><td>average_bitrate : <input type="text" name="{$stream}_average_bitrate" id="7"><xsl:attribute name="value"><xsl:value-of select="average_bitrate"/></xsl:attribute></input></td></tr>
</TABLE>
</xsl:template>

<xsl:template match="param/mjpeg">
<CAPTION>MJPEG</CAPTION>
<TABLE border="1" width="600">
 <tr><td>quality : <input type="text" name="mjpeg_quality" id="10"><xsl:attribute name="value"><xsl:value-of select="quality"/></xsl:attribute></input></td></tr>
</TABLE>
</xsl:template>

<xsl:template name="radiocontrol">
	<xsl:param name="group_name"/>
	<xsl:param name="radio_name"/>
	<xsl:param name="radio_value"/>
	<xsl:param name="current1"/>
		<input type="radio" name="$group_name" value="$radio_value" id="$radio_name">
		<xsl:if test="$current1= 'true'">
				<xsl:attribute name="checked"><xsl:value-of select="checked"/></xsl:attribute>
		</xsl:if>
		</input>
		<xsl:value-of select="."/>  <xsl:value-of select="$current1"/>
</xsl:template>

<xsl:template name="radiocontrol1">
	<xsl:param name="group_name"/>
	<xsl:param name="radio_name"/>
	<xsl:param name="current1"/>
		<input type="radio" name="{$group_name}" value="{$radio_name}" id="{$radio_name}">
		<xsl:if test="$current1= 'true'">
				<xsl:attribute name="checked"/>
		</xsl:if>
		</input>
		<xsl:value-of select="$radio_name"/>
</xsl:template>
<!--end-->
</xsl:stylesheet>

	<!--	<xsl:if test="@current[.='true']">
			<xsl:attribute name="checked"><xsl:value-of select="checked"/></xsl:attribute>-->


