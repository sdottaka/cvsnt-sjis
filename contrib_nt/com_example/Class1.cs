using System;
using System.Windows.Forms;
using CVSNT;

namespace cvscom
{
	/// <summary>
	/// Summary description for Class1.
	/// </summary>
	public class Class1 : CVSNT.ICvsInfo
	{
		public Class1()
		{
			//
			// TODO: Add constructor logic here
			//
		}
		#region ICvsInfo Members

		public short verifymsg(string filename)
		{
			// TODO:  Add Class1.verifymsg implementation
			return 0;
		}

		public short precommit(string repository, Array precommit_list)
		{
			// TODO:  Add Class1.precommit implementation
			return 0;
		}

		public short init(string repository, string username, string prefix, string sessionid, string hostname)
		{
			// TODO:  Add Class1.init implementation
			return 0;
		}

		public short notify(string short_repository, string file, string type, string repository, string who)
		{
			// TODO:  Add Class1.notify implementation
			return 0;
		}

		public short history(string repository, string history_line)
		{
			// TODO:  Add Class1.history implementation
			return 0;
		}

		public short loginfo(string repository, string hostname, string directory, string message, string status, Array file_list)
		{
			MessageBox.Show("file_list size is "+Convert.ToString(file_list.Length));
			ChangeInfoStruct st = (ChangeInfoStruct)file_list.GetValue(0);
			MessageBox.Show("first file is "+st.filename);
			return 0;
		}

		public short pretag(string tag, string action, string repository, Array pretag_list)
		{
			// TODO:  Add Class1.pretag implementation
			return 0;
		}

		public short postcommit(string repository)
		{
			// TODO:  Add Class1.postcommit implementation
			return 0;
		}

		#endregion
	}
}
