using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Documents;

namespace SimulationPlatform.Models;
public class ErrorMessage
{
    private int _code;
    private string _message;
    private readonly object _lock = new();


    public ErrorMessage(int code, string message)
    {
        Code = code;
        Message = message;
    }

    public int Code
    {
        get
        {
            lock (_lock) return _code;
        }
        set
        {
            lock (_lock) _code = value;
        }
    }

    public string Message
    {
        get
        {
            lock (_lock) return _message;
        }
        set
        {
            lock (_lock) _message = value;
        }
    }
}
