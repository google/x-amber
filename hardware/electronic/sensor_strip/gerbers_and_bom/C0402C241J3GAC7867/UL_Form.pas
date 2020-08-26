
procedure TUL_Form.BtnFileClick(Sender: TObject);
begin
	If OpenDlg.Execute Then Begin
    	TxtFile.Text := OpenDlg.FileName;
    End;
end;

procedure TUL_Form.BtnImportClick(Sender: TObject);
begin
	ImportAscIIData(TxtFile.Text);
    Close;
end;

