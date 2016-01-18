#include <stdio.h>

#include <string.h>

#include <errno.h>

#include "ClearSilver.h"
#include "mxml.h"

int AmbaVerif_process_PostData (HDF *hdf) {
	/*Verify the user name and password*/

	return 0;
}

int AmbaVerifUser (HDF *hdf) {
	int ret = AmbaVerif_process_PostData(hdf);
	char buffer[8];
	sprintf(buffer, "%d", ret);
	mxml_node_t *xml;
	mxml_node_t *data;
	mxml_node_t *node;

	xml = mxmlNewXML("1.0");
	data = mxmlNewElement(xml, "verification");
	node = mxmlNewElement(data, "res");
	mxmlElementSetAttr(node, "ul", buffer);
	printf("Content-type: application/xml\n\n");
	mxmlSaveFile(xml, stdout, MXML_NO_CALLBACK);
	mxmlDelete(xml);

	return ret;
}

int main()
{
	CGI *cgi = NULL;
	HDF *hdf = NULL;

	hdf_init(&hdf);
	cgi_init(&cgi, hdf);
	cgi_parse(cgi);

	AmbaVerifUser (hdf);
	//hdf_dump(hdf,"<br>");

	return 0;
}
