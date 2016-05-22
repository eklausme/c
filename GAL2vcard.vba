
' From https://msdn.microsoft.com/en-us/library/office/bb175075
Sub GAL2vcard()
    Dim colAL As Outlook.AddressLists
    Dim oAL As Outlook.AddressList
    Dim colAE As Outlook.AddressEntries
    Dim oAE As Outlook.AddressEntry
    Dim usr As Outlook.ExchangeUser
    Dim i As Long, j As Long, k As Long
    i = 0
    j = 0
    k = 0
    
    Set colAL = Application.Session.AddressLists
    Open "D:\klm\contacts.vcard" For Output Access Write As #1
    For Each oAL In colAL
        i = i + 1
        If i > 30 Then Exit For
        'Address list is an Exchange Global Address List
        If oAL.AddressListType = olExchangeGlobalAddressList Then
            Set colAE = oAL.AddressEntries
            For Each oAE In colAE
                j = j + 1
                If j > 125100 Then Exit For
                ' olExchangeUserAddressEntry=0: An Exchange user that belongs to the same Exchange forest.
                ' olExchangeRemoteUserAddressEntry=5: An Exchange user that belongs to a different Exchange forest.
                If oAE.AddressEntryUserType = 0 Then ' Or oAE.AddressEntryUserType = 5 Then
                    k = k + 1
                    If k > 53000 Then Exit For
                    Set usr = oAE.GetExchangeUser
                    Print #1, "BEGIN:VCARD"
                    Print #1, "VERSION:3.0"
                    Print #1, "CATEGORIES:Business"
                    Print #1, "FN:" & usr.FirstName & " " & usr.LastName
                    Print #1, "N:" & usr.LastName & ";" & usr.FirstName
                    Print #1, "ORG:" & usr.CompanyName
                    Print #1, "TEL;TYPE=WORK:" & usr.BusinessTelephoneNumber
                    Print #1, "TEL;TYPE=cell:" & usr.MobileTelephoneNumber
                    Print #1, "EMAIL:" & usr.PrimarySmtpAddress
                    Print #1, "ADR;TYPE=work:;;" & usr.StreetAddress
                    Print #1, "" & usr.PostalCode & " " & usr.City
                    Print #1, "END:VCARD"
                End If
            Next
        End If
    Next
    Close #1
    MsgBox "i=" & i & ", j=" & j & ", k=" & k

End Sub
