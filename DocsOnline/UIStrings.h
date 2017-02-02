wstring UIFormat(LPCTSTR formatString, ...);

#define lbl_queuepos                          L"N"
#define lbl_file                              L"File"
#define lbl_size                              L"Size"
#define lbl_status                            L"Status"
#define btn_upload                            L"Upload"
#define btn_stop                              L"Stop"
#define btn_clearlist                         L"Clear list"
#define btn_settings                          L"Settings..."
#define chk_convertdocs                       L"Convert documents"
#define hint_convertdocs                      L"Switches on document conversion into the Google format when down\n" \
                                              L"In conversion mode only a limited set of file types is accepted for upload\n" \
                                              L"Document conversion can be switched off only for Google Apps Premium accounts\n" \
                                              L"In premium account mode arbitrary files upload is allowed"
#define btn_exit                              L"Exit"
#define msg_selectfolder                      L"Select folder"

// Upload list context menu
#define mnu_uploadlistremove                  L"Remove from list"
#define mnu_uploadlistretry                   L"Retry upload"

#define msg_outofresources                    L"Out of system resources"
#define msg_authorizing                       L"Authorizing"
#define msg_processingfolders                 L"Processing folders"
#define msg_checkingforfilecopy               L"Checking for file copy"
#define msg_uploading                         L"Uploading"
#define msg_uploadingprogress                 L"Uploading: %1%%"
#define msg_stopped                           L"Stopped"
#define msg_uploadcompleted                   L"Uploaded"
#define msg_failed                            L"Failed"
#define msg_steperrorformat                   L"%1 - %2"

#define msg_winineterror                      L"WinInet error %1"
#define msg_srvnamenotresolved                L"The server name could not be resolved"
#define msg_requesttimeout                    L"The request has timed out"
#define msg_cannotconnect                     L"The attempt to connect to the server failed"
#define msg_connectionaborted                 L"The connection with the server has been terminated"
#define msg_connectionreset                   L"The connection with the server has been reset"
#define msg_invalidserverresponse             L"Invalid server response"
#define msg_securitychannelerror              L"Internal error loading the SSL libraries"

#define msg_postinputstreamerror              L"Internal error reading request stream"
#define msg_postoutputstreamerror             L"Internal error writing request result stream"

#define msg_httperror                         L"HTTP %1"
#define msg_httperrordescrformat              L"%1; %2"
#define msg_accessdenied                      L"Access denied"
#define msg_forbidden                         L"Forbidden"
#define msg_unsupportedmedia                  L"Unsupported media type"

#define msg_unexpectedserverreply             L"Error parsing server reply"

#define msg_fileopenerror                     L"Error opening file"
#define msg_filereaderror                     L"Error reading file"

#define msg_uploadsuccess                     L"Upload completed successfully"
#define msg_uploadwitherrors                  L"Upload completed. Some of the files could not be uploaded"
#define msg_uploadstoperror                   L"Upload stopped because of an error"

#define msg_clearlistinupload                 L"Clearing the list will abort the current upload, are you sure ?"
#define msg_exitinupload                      L"Exiting application now will abort the current upload, are you sure ?"
#define msg_xfilesqueued                      L"%1 file(s) queued for upload"
#define msg_xfilesofyqueued                   L"%1 file(s) out of %2 queued.\n%3 file(s) skipped because or file type/size restrictions or to avoid duplicate entries"
#define msg_nonequeued                        L"No files were queued\n%1 file(s) skipped because of file type/size restrictions or to avoid duplicate entries"

#define msg_uploaderkeepsworking              L"The application keeps working. You may click this icon to restore the application window"
#define msg_retrymessage_fmt                  L"%1) %2"
#define msg_filemathesservercopy              L"File matches server copy"
#define msg_filemathesservercopyas            L"File matches \"%1\""
#define msg_uploadedas                        L"Uploaded as \"%1\""
