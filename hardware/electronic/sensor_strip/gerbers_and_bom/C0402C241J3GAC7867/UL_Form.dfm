object UL_Form: TUL_Form
  Left = 0
  Top = 0
  BorderStyle = bsDialog
  Caption = 'UL Import'
  ClientHeight = 80
  ClientWidth = 473
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  PixelsPerInch = 96
  TextHeight = 13
  object TxtFile: TEdit
    Left = 80
    Top = 8
    Width = 384
    Height = 24
    TabOrder = 0
  end
  object BtnFile: TButton
    Left = 8
    Top = 8
    Width = 64
    Height = 24
    Caption = 'File...'
    TabOrder = 1
    OnClick = BtnFileClick
  end
  object BtnImport: TButton
    Left = 188
    Top = 40
    Width = 96
    Height = 32
    Caption = 'Start Import'
    TabOrder = 2
    OnClick = BtnImportClick
  end
  object OpenDlg: TOpenDialog
    DefaultExt = '.txt'
    Filter = 'UL Altium Output (*.txt)|*.txt'
    Left = 8
    Top = 40
  end
end
